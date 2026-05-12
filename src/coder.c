/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 10:47:53 by acombier          #+#    #+#             */
/*   Updated: 2026/05/12 11:42:32 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

  /*                                                                                                                                                                                                     
  ** coder_take_dongles
  ** Strategie anti-deadlock (option B) :                                                                                                                                                                
  **   - id pair  : gauche d abord, puis droite
  **   - id impair: droite d abord, puis gauche                                                                                                                                                          
  ** Cette asymetrie casse la condition de "hold and wait circulaire"
  ** (une des 4 conditions de Coffman) => deadlock impossible.                                                                                                                                           
  */

void	coder_take_dongles(t_coder *coder)
{
	if(coder->id % 2 == 0)
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

  /*
  ** coder_release_dongles
  ** Libere les deux dongles en ORDRE INVERSE de l acquisition (LIFO).
  ** Pourquoi LIFO : minimise les inversions de priorite dans la file                                                                                                                                    
  ** d attente du dongle.                                                                                                                                                                                
  */

void	coder_release_dongles(t_coder *coder)
{
	if(coder->id % 2 == 0)
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

  /*              
  ** coder_do_cycle
  ** Un cycle complet : compile -> debug -> refactor.
  ** Points critiques :                                                                                                                                                                                  
  **   - last_compile_start_ms est mis a jour SOUS coder_state_mutex AVANT
  **     le log "is compiling" : si on inversait l ordre, le monitor pourrait                                                                                                                            
  **     lire une valeur trop ancienne et declarer un burnout faussement.                                                                                                                                
  **   - On libere les dongles AVANT debug/refactor : ces deux phases ne                                                                                                                                 
  **     necessitent pas la ressource, liberer tot maximise le throughput.                                                                                                                               
  **   - compiles_done++ sous mutex pour eviter un torn read 64 bits.                                                                                                                                    
  */

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

  /*
  ** coder_should_continue
  ** Decide si le coder doit refaire un cycle. Deux raisons d arreter :
  **   1. sim->stop a ete mis a 1 (par le monitor : burnout ou fin atteinte)
  **   2. compiles_done a atteint number_of_compiles_required
  ** Chaque lecture sous son mutex pour la coherence.
  */

int	coder_should_continue(t_coder *coder)
{
	t_sim	*sim;
	int		stop;
	int		done;
	
	sim = coder->sim;
	pthread_mutex_lock(&sim->stop_mutex);
	stop = sim->stop;
	pthread_mutex_unlock(&sim->stop_mutex);
	if(stop)
		return (0);
	pthread_mutex_lock(&sim->coder_state_mutex);
	done = coder->compiles_done;
	pthread_mutex_unlock(&sim->coder_state_mutex);
	return (done < sim->config.number_of_compiles_required);
}

  /*
  ** coder_routine
  ** Point d entree du thread coder (signature void *(void *) imposee par pthread).
  ** Initialise last_compile_start_ms a sim->start_ms : si on initialisait a 0,
  ** le monitor verrait un "since_last_compile" = now - 0 enorme et declarerait
  ** un burnout immediat des le 1er tour.
  ** Sous coder_state_mutex car le monitor peut deja tourner.
  */


void	*coder_routine(void *arg)
{
	t_coder *coder;
	
	coder = (t_coder *)arg;
	pthread_mutex_lock(&coder->sim->coder_state_mutex);
	coder->last_compile_start_ms = coder->sim->start_ms;
	pthread_mutex_unlock(&coder->sim->coder_state_mutex);
	while(coder_should_continue(coder))
		coder_do_cycle(coder);
	return (NULL);
}


