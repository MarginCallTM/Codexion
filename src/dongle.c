/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 12:26:28 by acombier          #+#    #+#             */
/*   Updated: 2026/05/13 16:16:12 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

 /*              
  ** Initialise un dongle : id, champs a zero, heap de waiters, mutex, cond.
  ** Retourne 0 si tout va bien, 1 en cas d echec d allocation.                                                                                                                                                    
  **                                                                                                                                                                                                               
  ** Ordre d init important pour le cleanup en cas d erreur :                                                                                                                                                      
  **   1. pq_init  -> si echoue, rien d autre a detruire                                                                                                                                                           
  **   2. mutex    -> si echoue, pq_destroy                                                                                                                                                                        
  **   3. cond     -> si echoue, pq_destroy + mutex_destroy                                                                                                                                                        
  **                                                                                                                                                                                                               
  ** released_at_ms = 0 signifie "jamais relache" (pas de cooldown a l init).
  ** pq_init(4) : capacite initiale de 4 noeuds (croissance dynamique ensuite).                                                                                                                                    
  */

int		dongle_init(t_dongle *d, int id)
{
	d->id = id;
	d->taken = 0;
	d->released_at_ms = 0;
  d->stop_requested = 0;
	d->waiters = pq_init(4);
	if (!d->waiters)
		return (1);
	if (pthread_mutex_init(&d->mutex, NULL) != 0)
	{
		pq_destroy(d->waiters);
		return (1);
	}
	if (pthread_cond_init(&d->cond, NULL) != 0)
	{
		pq_destroy(d->waiters);
		pthread_mutex_destroy(&d->mutex);
		return (1);
	}
	return (0);
}

 /*              
  ** Libere toutes les ressources d un dongle.
  ** Appele par sim_destroy apres que tous les threads sont joints.
  ** Ordre inverse de l init : cond -> mutex -> heap.                                                                                                                                                              
  */

void	dongle_destroy(t_dongle *d)
{
	pq_destroy(d->waiters);
	pthread_mutex_destroy(&d->mutex);
	pthread_cond_destroy(&d->cond);
}

  /*
  ** Prend possession d un dongle (bloquant).
  **                                                                                                                                                                                                               
  ** Ordre des operations :
  **   1. Calcul de la cle HORS mutex (voir dongle_compute_key)                                                                                                                                                    
  **   2. Lock d->mutex : zone critique, on touche la heap et taken                                                                                                                                                
  **   3. On s inscrit dans la file (pq_push) AVANT d entrer en attente                                                                                                                                            
  **      -> garantit qu on ne rate aucun broadcast pendant le push                                                                                                                                                
  **   4. Boucle d attente (dongle_wait_loop)                                                                                                                                                                      
  **   5. On se retire de la file (pq_remove) : on a le dongle, on n attend plus                                                                                                                                   
  **   6. taken = 1 : marque la prise AVANT d unlock pour eviter une                                                                                                                                               
  **      fenetre ou un autre coder nous vole le dongle apres notre sortie                                                                                                                                         
  **      de wait_loop mais avant taken=1                                                                                                                                                                          
  **   7. Unlock d->mutex                                                                                                                                                                                          
  **   8. Log APRES unlock : log_taken_dongle a besoin de log_mutex.                                                                                                                                               
  **      Or log_mutex doit etre pris AVANT dongle.mutex (ordre de lock).                                                                                                                                          
  **      Donc on NE PEUT PAS logger sous d->mutex.                                                                                                                                                                
  */

void	dongle_request(t_coder *coder, t_dongle *d)
{
	long long	key;
	t_sim	*sim;

	sim = coder->sim;
	key = dongle_compute_key(coder, sim);
	pthread_mutex_lock(&d->mutex);
	pq_push(d->waiters, coder->id, key);
	dongle_wait_loop(d, coder, sim);
	pq_remove(d->waiters, coder->id);
	d->taken = 1;
	pthread_mutex_unlock(&d->mutex);
	log_taken_dongle(sim, coder->id);
}

 /*                                                                                                                                                                                                               
  ** Relache un dongle apres utilisation.
  **                                                                                                                                                                                                               
  ** released_at_ms mis a jour AVANT le broadcast : si un thread se reveille
  ** immediatement apres le broadcast, il lira le bon timestamp dans                                                                                                                                               
  ** dongle_can_take pour calculer le cooldown restant.                                                                                                                                                            
  **                                                                                                                                                                                                               
  ** pthread_cond_broadcast (pas signal) : obligatoire selon le sujet.                                                                                                                                             
  ** Tous les coders en attente se reveillent et re-evaluent can_take.                                                                                                                                             
  ** Seul celui qui est en tete de file ET dont le cooldown est ecoule passera.                                                                                                                                    
  */

void	dongle_release(t_dongle *d)
{
	pthread_mutex_lock(&d->mutex);
	d->taken = 0;
	d->released_at_ms = now_ms();
	pthread_cond_broadcast(&d->cond);
	pthread_mutex_unlock(&d->mutex);
}