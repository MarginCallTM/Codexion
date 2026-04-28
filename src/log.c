/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 16:08:16 by acombier          #+#    #+#             */
/*   Updated: 2026/04/28 17:04:52 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

  /*              
  ** Logger generique. Format impose par le sujet : "<ts> <id> <msg>".
  **
  ** ORDRE DES LOCKS du projet (a respecter PARTOUT) :
  **   log_mutex -> stop_mutex -> coder_state_mutex -> dongle.mutex                                                         
  **                                                                                                                        
  ** Pourquoi prendre log_mutex AVANT de checker stop ? Si on checkait stop                                                 
  ** sans lock, deux threads pourraient passer la verif (stop == 0) puis                                                    
  ** entrer en region critique et imprimer apres que le monitor a deja
  ** loggue "burned out". Le log_mutex sert de point de serialisation des                                                   
  ** decisions "j imprime ou pas".                                                                                          
  */

void	log_event(t_sim *sim, int coder_id, const char *msg)
{
	long long	ts;
	
	pthread_mutex_lock(&sim->log_mutex);
	pthread_mutex_lock(&sim->stop_mutex);
	if(sim->stop)
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

  /* Wrappers avec les chaines EXACTES du sujet. Ne jamais modifier le                                                      
  ** texte sans verifier le sujet : les correcteurs grep les chaines. */

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