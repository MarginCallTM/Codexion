/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 12:24:06 by acombier          #+#    #+#             */
/*   Updated: 2026/05/13 16:16:55 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

    /*
    ** monitor_signal_all_dongles
    ** Reveille tous les coders en attente sur cond_timedwait. Indispensable
    ** pour qu ils sortent vite quand sim->stop passe a 1 : sans ce broadcast,
    ** un coder bloque dans dongle_wait_loop attendrait jusqu au prochain
    ** timeout (mauvaise reactivite, log "burned out" affiche bien apres).
    ** On ne detient aucun autre lock ici => ordre log -> stop -> coder
    ** -> dongle respecte.
    */

void	monitor_signal_all_dongles(t_sim *sim)
{
	int		i;

	i = 0;
	while(i < sim->config.number_of_coders)
	{
		pthread_mutex_lock(&sim->dongles[i].mutex);
		sim->dongles[i].stop_requested = 1;
		pthread_cond_broadcast(&sim->dongles[i].cond);
		pthread_mutex_unlock(&sim->dongles[i].mutex);
		i++;
	}
}

/*
    ** monitor_check_burnout
    ** Parcourt les coders : si (now - last_compile_start_ms) > time_to_burnout,
    ** retourne l id de ce coder ; sinon -1.
    ** Lecture de last_compile_start_ms et compiles_done SOUS coder_state_mutex
    ** (torn read 64 bits possible sinon selon l archi). now_ms() en dehors du
    ** lock : c est un syscall, inutile de tenir le mutex pendant.
    ** Coders deja finis (compiles_done >= required) sont ignores : leur
    ** thread est sorti, last_compile_start_ms est fige, le monitor ne doit
    ** pas declencher un burnout artificiel pendant que les autres terminent.
*/

int		monitor_check_burnout(t_sim *sim)
{
	int		i;
	long long		last;
	long long		since;
	int		done;

	i = 0;
	while(i < sim->config.number_of_coders)
	{
		pthread_mutex_lock(&sim->coder_state_mutex);
		last = sim->coders[i].last_compile_start_ms;
		done = sim->coders[i].compiles_done;
		pthread_mutex_unlock(&sim->coder_state_mutex);
		since = now_ms() - last;
		if(done < sim->config.number_of_compiles_required && since > sim->config.time_to_burnout)
			return (sim->coders[i].id);
		i++;
	}
	return (-1);
}

/*
** monitor_check_end
** Retourne 1 si TOUS les coders ont fait au moins
** number_of_compiles_required compilations. Sortie anticipee des qu un
** coder n a pas encore fini : evite de tenir le mutex inutilement.
*/

int		monitor_check_end(t_sim *sim)
{
	int		i;
	int		done;

	i = 0;
	
	while(i < sim->config.number_of_coders)
	{
		pthread_mutex_lock(&sim->coder_state_mutex);
		done = sim->coders[i].compiles_done;
		pthread_mutex_unlock(&sim->coder_state_mutex);
		if(done < sim->config.number_of_compiles_required)
			return(0);
		i++;
	}
	return (1);
}


/*
    ** monitor_trigger_burnout
    ** log_burned_out fait DEUX choses sous log_mutex : ecrire "burned out"
    ** ET set sim->stop = 1, atomiquement (cf. log_burnout.c). Apres ca on
    ** broadcast sur tous les dongles pour reveiller les attendants.
    ** log_burned_out a deja relache log_mutex => l acquisition des dongle
    ** mutexes qui suit ne viole pas l ordre des locks.
*/

void	monitor_trigger_burnout(t_sim *sim, int coder_id)
{
	log_burned_out(sim, coder_id);
	monitor_signal_all_dongles(sim);
}


 /*
    ** monitor_routine
    ** Boucle de detection. Trois sorties :
    **   1. Un coder burnout       -> trigger_burnout (logge + stop=1) + return
    **   2. Tous ont fini compiles -> stop=1 + broadcast + return
    **   3. stop deja a 1          -> return (defensif)
    ** usleep(500) = 0.5 ms : tres en dessous des 10 ms de precision exiges
    ** par le sujet pour la detection du burnout.
*/

void	*monitor_routine(void *arg)
{
	t_sim	*sim;
	int		burnt;
	sim = (t_sim *)arg;
	while(1)
	{
		burnt = monitor_check_burnout(sim);
		if(burnt != -1)
			return (monitor_trigger_burnout(sim, burnt), NULL);
		if(monitor_check_end(sim))
		{
			pthread_mutex_lock(&sim->stop_mutex);
			sim->stop = 1;
			pthread_mutex_unlock(&sim->stop_mutex);
			monitor_signal_all_dongles(sim);
			return (NULL);
		}
		usleep(500);
	}
	return (NULL);
}
