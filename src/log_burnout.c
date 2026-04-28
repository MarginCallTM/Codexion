/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log_burnout.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 16:21:52 by acombier          #+#    #+#             */
/*   Updated: 2026/04/28 17:04:23 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

 /*              
  ** Cas special : le burnout DOIT etre logge meme (et surtout) si stop
  ** vient d etre mis a 1. Donc on duplique la logique de log_event sans                                                    
  ** le check "if (sim->stop) return".
  **                                                                                                                        
  ** Cette fonction fait DEUX choses atomiquement, sous log_mutex :
  **   1. log "<ts> <id> burned out"                                                                                        
  **   2. set sim->stop = 1
  ** Ensemble, sous le meme lock, pour qu aucun autre log ne puisse                                                         
  ** s intercaler entre l ecriture du burnout et la propagation du stop.                                                    
  **                                                                                                                        
  ** Le check "if (sim->stop) return" en debut sert a un autre cas : si                                                     
  ** plusieurs coders burnent simultanement, seul le premier doit imprimer                                                  
  ** "burned out". Les suivants voient stop deja a 1 et abandonnent                                                         
  ** silencieusement (le burnout du premier coder declenche l arret).                                                       
  **                                                                                                                        
  ** Ordre des locks respecte : log_mutex puis stop_mutex.                                                                  
  */

void	log_burned_out(t_sim *sim, int coder_id)
{
	long long	ts;
	
	pthread_mutex_lock(&sim->log_mutex);
	pthread_mutex_lock(&sim->stop_mutex);
	if(sim->stop)
	{
		pthread_mutex_unlock(&sim->stop_mutex);
		pthread_mutex_unlock(&sim->log_mutex);
		return;
	}
	sim->stop = 1;
	pthread_mutex_unlock(&sim->stop_mutex);
	ts = elapsed_ms(sim);
	printf("%lld %d %s", ts, coder_id, "burned out\n");
	pthread_mutex_unlock(&sim->log_mutex);
}
