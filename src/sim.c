/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 15:07:39 by acombier          #+#    #+#             */
/*   Updated: 2026/05/13 15:58:46 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

 /*
    ** sim_init_mutexes
    ** Initialise les 3 mutex de t_sim. Cleanup en cascade en cas d echec :
    ** si le 2e echoue, on detruit le 1er ; si le 3e echoue, on detruit les
    ** 2 premiers. Apres retour 0, les 3 mutex sont garantis utilisables.
*/

int		sim_init_mutexes(t_sim *sim)
{
	if(pthread_mutex_init(&sim->stop_mutex, NULL) != 0)
		return (1);
	if(pthread_mutex_init(&sim->log_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->stop_mutex);
		return (1);
	}
	if(pthread_mutex_init(&sim->coder_state_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&sim->stop_mutex);
		pthread_mutex_destroy(&sim->log_mutex);
		return (1);
	}
	return (0);
}

/*
    ** sim_init_dongles
    ** Alloue le tableau de N dongles puis les initialise un par un.
    ** Si un dongle_init echoue : detruit ceux deja crees, free le tableau,
    ** remet sim->dongles a NULL pour que sim_destroy soit safe.
*/

int		sim_init_dongles(t_sim *sim)
{
	int		i;
	int		n;

	n = sim->config.number_of_coders;
	sim->dongles = malloc(sizeof(t_dongle) * n);
	if(!sim->dongles)
		return (1);
	i = 0;
	while(i < n)
	{
		if(dongle_init(&sim->dongles[i], i) != 0)
		{
			while(--i >= 0)
				dongle_destroy(&sim->dongles[i]);
			free(sim->dongles);
			sim->dongles = NULL;
			return(1);
		}
		i++;
	}
	return (0);
}

/*
    ** sim_init_coders
    ** Alloue le tableau de N coders et remplit chaque champ. Topologie :
    ** coder[i].left = dongle[i], coder[i].right = dongle[(i+1) % N]
    ** -> chaque dongle est partage entre 2 coders adjacents (config table
    ** ronde des philosophes). last_compile_start_ms sera fixe par sim_init
    ** APRES start_ms = now_ms(). Pas de cleanup partiel : un seul malloc.
*/


int		sim_init_coders(t_sim *sim)
{
	int		i;
	int		n;

	n = sim->config.number_of_coders;
	sim->coders = malloc(sizeof(t_coder) * n );
	if(!sim->coders)
		return(1);
	i = 0;
	while(i < n)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].state = STATE_REFACTORING;
		sim->coders[i].compiles_done = 0;
		sim->coders[i].last_compile_start_ms = 0;
		sim->coders[i].left = &sim->dongles[i];
		sim->coders[i].right = &sim->dongles[(i + 1) % n];
		sim->coders[i].sim = sim;
		i++;
	}
	return (0);
}



 /*
    ** sim_init
    ** Orchestrateur. memset a 0 d abord -> tous les pointeurs NULL,
    ** sim_destroy idempotent. Sequence : mutexes -> dongles -> coders.
    ** En cas d echec a une etape, sim_destroy nettoie ce qui a deja ete
    ** initialise (les NULL sont skip). start_ms est fixe APRES toutes les
    ** allocs (pour minimiser le decalage avec le lancement reel des threads).
    ** Boucle finale : init last_compile_start_ms = start_ms sur TOUS les
    ** coders, AVANT que main lance le monitor (sinon faux burnout immediat).
*/

int		sim_init(t_sim *sim, const t_config *cfg)
{
	int		i;

	memset(sim, 0, sizeof(*sim));
	sim->config = *cfg;
	if(sim_init_mutexes(sim) != 0)
		return (1);
	if(sim_init_dongles(sim) != 0)
		return (sim_destroy(sim), 1);
	if(sim_init_coders(sim) != 0)
		return(sim_destroy(sim), 1);
	sim->start_ms = now_ms();
	i = 0;
	while(i < sim->config.number_of_coders)
	{
		sim->coders[i].last_compile_start_ms = sim->start_ms;
		i++;
	}
	return (0);
}

/*
    ** sim_destroy
    ** Idempotent : chaque ressource est verifiee (pointeur != NULL) avant
    ** destruction. Permet d etre appele depuis sim_init en cas d echec
    ** partiel SANS connaitre quel etage a echoue. Mutexes detruits en
    ** dernier (les autres ressources peuvent en avoir besoin pendant leur
    ** propre destruction).
    ** Pre-condition : sim_init_mutexes a reussi (sinon on n appelle pas
    ** sim_destroy du tout, sim_init_mutexes nettoie son propre echec).
*/

void	sim_destroy(t_sim *sim)
{
	int		i;

	if(sim->coders)
	{
		free(sim->coders);
		sim->coders = NULL;
	}
	if(sim->dongles)
	{
		i = 0;
		while(i < sim->config.number_of_coders)
		{
			dongle_destroy(&sim->dongles[i]);
			i++;
		}
		free(sim->dongles);
		sim->dongles = NULL;
	}
	pthread_mutex_destroy(&sim->stop_mutex);
	pthread_mutex_destroy(&sim->log_mutex);
	pthread_mutex_destroy(&sim->coder_state_mutex);
}
