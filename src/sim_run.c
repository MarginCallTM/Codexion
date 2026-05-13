/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_run.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 15:51:37 by acombier          #+#    #+#             */
/*   Updated: 2026/05/13 15:53:15 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/*
    ** sim_emergency_stop
    ** Appelee si un pthread_create echoue au milieu du lancement des coders.
    ** "created" = nb de threads coders deja lances avec succes (le monitor
    ** est forcement deja lance, on est ici uniquement si la boucle coders a
    ** plante). Sequence : set stop=1, broadcast pour reveiller les coders
    ** deja lances qui pourraient attendre un dongle, puis join tout le monde
    ** pour eviter une fuite de ressource thread (et faire taire valgrind).
*/

void	sim_emergency_stop(t_sim *sim, int created)
{
	pthread_mutex_lock(&sim->stop_mutex);
	sim->stop = 1;
	pthread_mutex_unlock(&sim->stop_mutex);
	monitor_signal_all_dongles(sim);
	while(--created >= 0)
		pthread_join(sim->coders[created].thread, NULL);
	pthread_join(sim->monitor, NULL);
}

/*
    ** sim_launch
    ** Lance le monitor EN PREMIER (TODO 10.1.4) : il doit etre pret a
    ** detecter le burnout des le tout debut. Puis chaque coder.
    ** usleep(100) entre chaque coder : evite que tous prennent leur ticket
    ** FIFO simultanement -> ordre plus deterministe pour les tests
    ** (sans cet usleep le code reste correct, juste moins reproductible).
    ** Si un pthread_create coder echoue, sim_emergency_stop fait le menage.
*/

int		sim_launch(t_sim *sim)
{
	int		i;
	if(pthread_create(&sim->monitor, NULL, monitor_routine, sim) != 0)
		return (1);
	i = 0;
	while(i < sim->config.number_of_coders)
	{
		if(pthread_create(&sim->coders[i].thread, NULL,
				coder_routine, &sim->coders[i]) != 0)
					return (sim_emergency_stop(sim, i), 1);
		usleep(100);
		i++;
	}
	return (0);
}

/*
    ** sim_join
    ** Attend la fin de tous les coders puis du monitor.
    ** Ordre : coders d abord (ils finissent quand stop=1 ou quand ils ont
    ** atteint number_of_compiles_required), monitor en dernier (le monitor
    ** sort de sa boucle quand il a detecte burnout ou fin -> il termine de
    ** lui-meme apres avoir set stop=1 et broadcast).
    ** Pas de check de retour : pthread_join echoue uniquement sur des
    ** erreurs de programmation (thread invalide), pas sur des conditions
    ** runtime.
*/

void	sim_join(t_sim *sim)
{
	int		i;
	
	i = 0;
	while(i < sim->config.number_of_coders)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	pthread_join(sim->monitor, NULL);
}
