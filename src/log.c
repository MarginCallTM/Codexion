/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 16:08:16 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 15:17:04 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Display the coders events and avoid overlay with mutex
void	log_event(t_sim *sim, int coder_id, const char *msg)
{
	long long	ts;

	pthread_mutex_lock(&sim->log_mutex);
	pthread_mutex_lock(&sim->stop_mutex);
	if (sim->stop)
	{
		pthread_mutex_unlock(&sim->stop_mutex);
		pthread_mutex_unlock(&sim->log_mutex);
		return ;
	}
	pthread_mutex_unlock(&sim->stop_mutex);
	ts = elapsed_ms(sim);
	printf("%lld %d %s\n", ts, coder_id, msg);
	pthread_mutex_unlock(&sim->log_mutex);
}

// Wrapper logs
void	log_taken_dongle(t_sim *sim, int coder_id)
{
	log_event(sim, coder_id, "has taken a dongle");
}

void	log_compiling(t_sim *sim, int coder_id)
{
	log_event(sim, coder_id, "is compiling");
}

void	log_debugging(t_sim *sim, int coder_id)
{
	log_event(sim, coder_id, "is debugging");
}

void	log_refactoring(t_sim *sim, int coder_id)
{
	log_event(sim, coder_id, "is refactoring");
}
