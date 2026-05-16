/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 10:47:53 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 18:56:34 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Take 2 dongles for compiling Right Left or Left Right
void	coder_take_dongles(t_coder *coder)
{
	if (coder->id % 2 == 0)
	{
		dongle_request(coder, coder->left);
		dongle_request(coder, coder->right);
	}
	else
	{
		dongle_request(coder, coder->right);
		dongle_request(coder, coder->left);
	}
}

// Release 2 dongles
void	coder_release_dongles(t_coder *coder)
{
	if (coder->id % 2 == 0)
	{
		dongle_release(coder->right);
		dongle_release(coder->left);
	}
	else
	{
		dongle_release(coder->left);
		dongle_release(coder->right);
	}
}

// Execute compile -> Debug -> Refactor
void	coder_do_cycle(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	coder_take_dongles(coder);
	pthread_mutex_lock(&sim->coder_state_mutex);
	coder->last_compile_start_ms = now_ms();
	pthread_mutex_unlock(&sim->coder_state_mutex);
	log_compiling(sim, coder->id);
	precise_sleep_ms(sim->config.time_to_compile, sim);
	pthread_mutex_lock(&sim->coder_state_mutex);
	coder->compiles_done++;
	pthread_mutex_unlock(&sim->coder_state_mutex);
	coder_release_dongles(coder);
	log_debugging(sim, coder->id);
	precise_sleep_ms(sim->config.time_to_debug, sim);
	log_refactoring(sim, coder->id);
	precise_sleep_ms(sim->config.time_to_refactor, sim);
}

// Check stop and number of compilation
int	coder_should_continue(t_coder *coder)
{
	t_sim	*sim;
	int		stop;
	int		done;

	sim = coder->sim;
	pthread_mutex_lock(&sim->stop_mutex);
	stop = sim->stop;
	pthread_mutex_unlock(&sim->stop_mutex);
	if (stop)
		return (0);
	pthread_mutex_lock(&sim->coder_state_mutex);
	done = coder->compiles_done;
	pthread_mutex_unlock(&sim->coder_state_mutex);
	return (done < sim->config.number_of_compiles_required);
}

// Main function execute at each thread
void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	pthread_mutex_lock(&coder->sim->coder_state_mutex);
	coder->last_compile_start_ms = coder->sim->start_ms;
	pthread_mutex_unlock(&coder->sim->coder_state_mutex);
	while (coder_should_continue(coder))
		coder_do_cycle(coder);
	return (NULL);
}
