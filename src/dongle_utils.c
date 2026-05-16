/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 12:10:37 by acombier          #+#    #+#             */
/*   Updated: 2026/05/13 16:17:35 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

  /*
  ** Calcule la cle de priorite pour la heap selon le scheduler :
  **
  ** FIFO : (compiles_done * N) + coder_id
  **   -> primaire : nombre de compiles deja faits (moins = priorite haute)
  **   -> secondaire : coder_id (deterministe, evite la dependance au
  **      scheduler kernel sur l ordre des pthread_create)
  **   -> garantit anti-starvation : un coder qui n a JAMAIS compile
  **      bat toujours un coder qui en a fait au moins un, peu importe
  **      l ordre des pushes.
  **   -> lock coder_state_mutex pour lire compiles_done de facon sure.
  **
  ** EDF (Earliest Deadline First) :
  **   -> deadline = debut du dernier compile + temps_max_sans_compiler
  **   -> le coder dont le burnout est le plus PROCHE passe en premier
  **
  ** IMPORTANT : la cle est calculee AVANT de locker d->mutex.
  ** Pourquoi ? Parce qu on doit prendre coder_state_mutex pour calculer
  ** la cle, qui est AVANT dongle.mutex dans l ordre de lock.
  ** On ne peut pas les prendre APRES.
  */

long long	dongle_compute_key(t_coder *coder, t_sim *sim)
{
	long long	key;

	if (sim->config.scheduler == SCHED_FIFO_)
	{
		pthread_mutex_lock(&sim->coder_state_mutex);
		key = (long long)coder->compiles_done
			* (long long)sim->config.number_of_coders + coder->id;
		pthread_mutex_unlock(&sim->coder_state_mutex);
	}
	else
	{
		pthread_mutex_lock(&sim->coder_state_mutex);
		key =  coder->last_compile_start_ms + sim->config.time_to_burnout;
		pthread_mutex_unlock(&sim->coder_state_mutex);
	}
	return (key);
}

 /*
  ** Retourne 1 si coder_id peut prendre le dongle d maintenant, 0 sinon.
  ** Appele depuis dongle_wait_loop sous d->mutex.                                                                                                                                                                 
  **                                                                                                                                                                                                               
  ** 3 conditions toutes obligatoires :                                                                                                                                                                            
  **  1. dongle libre  (!taken)                                                                                                                                                                                    
  **  2. cooldown ecoule : now - released_at >= cooldown                                                                                                                                                           
  **     Exception : released_at == 0 signifie "jamais relache" -> pas de cooldown                                                                                                                                 
  **  3. on est en tete de file : pq_peek renvoie notre coder_id                                                                                                                                                   
  **                                                                                                                                                                                                               
  ** Pourquoi verifier released_at_ms > 0 ?                                                                                                                                                                        
  ** released_at_ms est initialise a 0 dans dongle_init (pas a now_ms()).                                                                                                                                          
  ** Si on ne checkait pas, elapsed = now_ms() - 0 serait enorme (>cooldown)                                                                                                                                       
  ** ce qui serait accidentellement correct... mais only by luck.                                                                                                                                                  
  ** La verification explicite rend l intention claire.                                                                                                                                                            
  */

int		dongle_can_take(t_dongle *d, int coder_id, long long cooldown)
{
	t_pqnode	top;
	long long	elapsed;
	
	if (d->taken)
		return (0);
	elapsed = now_ms() - d->released_at_ms;
	if (d->released_at_ms > 0 && elapsed < cooldown)
		return (0);
	if (pq_peek(d->waiters, &top) != 0)
		return (0);
	return (top.coder_id == coder_id);
}

/*                                                                                                                                                                                                               
  ** Boucle d attente sous d->mutex jusqu a pouvoir prendre le dongle.
  **                                                                                                                                                                                                               
  ** Pourquoi while et pas if ?
  **   1. pthread_cond_wait peut avoir des spurious wakeups (spec POSIX).                                                                                                                                          
  **   2. Un broadcast reveille TOUS les waiters, mais un seul peut prendre                                                                                                                                        
  **      le dongle. Les autres doivent re-tester et se rendormir.                                                                                                                                                 
  **                                                                                                                                                                                                               
  ** Strategie timedwait :                                                                                                                                                                                         
  **   Si le dongle est libre mais en cooldown, on connait exactement                                                                                                                                              
  **   quand le cooldown expire -> on utilise timedwait(remaining) pour                                                                                                                                            
  **   se reveiller pile a ce moment sans attendre un broadcast qui ne                                                                                                                                             
  **   viendrait peut-etre jamais.                                                                                                                                                                                 
  **   Sinon (dongle pris, ou pas en tete de file) : fallback 2ms.                                                                                                                                                 
  **   Ce fallback de 2ms sert aussi de filet de securite : si on rate                                                                                                                                             
  **   le broadcast du monitor (race benigne sur sim->stop), on sortira                                                                                                                                            
  **   de la boucle au plus 2ms apres.                                                                                                                                                                             
  **                                                                                                                                                                                                               
  ** Race sur sim->stop : lu sans lock car :                                                                                                                                                                       
  **   - on est sous d->mutex, on NE PEUT PAS prendre stop_mutex                                                                                                                                                   
  **     (violerait l ordre log->stop->coder_state->dongle)                                                                                                                                                        
  **   - le monitor broadcast sur tous les conds apres avoir mis stop=1                                                                                                                                            
  **   - pire cas : on relit la boucle une fois de trop, puis on sort                                                                                                                                              
  */

void	dongle_wait_loop(t_dongle *d, t_coder *coder, t_sim *sim)
{
	struct timespec	ts;
	long long		remaining;

	while (!d->stop_requested
		&& !dongle_can_take(d, coder->id, sim->config.dongle_cooldown))
	{
		remaining = d->released_at_ms
			+ sim->config.dongle_cooldown - now_ms();
		if (!d->taken && d->released_at_ms > 0 && remaining > 0)
			ms_to_timespec(remaining, &ts);
		else
			ms_to_timespec(2, &ts);
		pthread_cond_timedwait(&d->cond, &d->mutex, &ts);
	}
}