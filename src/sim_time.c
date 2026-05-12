/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_time.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 16:03:43 by acombier          #+#    #+#             */
/*   Updated: 2026/04/29 12:49:30 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

 /*              
  ** Temps actuel en millisecondes depuis l epoch UNIX.
  ** gettimeofday remplit tv_sec (secondes) + tv_usec (microsecondes).                                                      
  ** Pourquoi long long ? Un timestamp epoch en ms vaut ~1.7e12, ne tient
  ** pas dans un int 32 bits. Le suffixe LL force la constante a etre                                                       
  ** long long pour eviter une multiplication 32 bits avec overflow.                                                        
  */
long long	now_ms(void)
{
	struct timeval	tv;
	
	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL);
}

/*                                                                                                                        
  ** Temps ecoule depuis le debut de la simulation, en ms.
  ** sim->start_ms est fixe une fois pour toutes par main() au lancement,
  ** donc cette soustraction donne un timestamp RELATIF (0, 200, 400...)                                                    
  ** plus lisible que l epoch absolu dans les logs.                                                                         
  */
long long	elapsed_ms(t_sim *sim)
{
	return (now_ms() - sim->start_ms);
}

  /*              
  ** Dors pendant exactement `ms` millisecondes, mais verifie sim->stop                                                                                                                                            
  ** tous les 100 µs pour sortir vite si la simulation s arrete.                                                                                                                                                   
  **                                                                                                                                                                                                               
  ** Pourquoi une boucle et pas un simple usleep(ms * 1000) ?                                                                                                                                                      
  ** usleep n est pas precis : le kernel peut nous reveiller en retard.                                                                                                                                            
  ** En boucle avec la vraie horloge on ne dort jamais PLUS que prevu.                                                                                                                                             
  **                                                                                                                                                                                                               
  ** Race benigne sur sim->stop : on lit sans lock car aucun autre mutex                                                                                                                                           
  ** n est tenu pendant ce sleep, et le cout d un faux negatif est juste                                                                                                                                           
  ** 100 µs de plus de sommeil.                                                                                                                                                                                    
  */

void	precise_sleep_ms(long long ms, t_sim *sim)
{
	long long	target;

	target = now_ms() + ms;
	while (!sim->stop && now_ms() < target)
		usleep(100);
}

/*                                                                                                                                                                                                               
  ** Construit un struct timespec absolu = maintenant + delta_ms.
  ** Utilise par pthread_cond_timedwait qui attend un TEMPS ABSOLU,                                                                                                                                                
  ** pas un delta (contrairement a nanosleep/usleep).
  **                                                                                                                                                                                                               
  ** tv_nsec = millisecondes restantes converties en nanosecondes.
  ** Le suffixe 1000000LL est obligatoire : 999 * 1000000 = 999 000 000,                                                                                                                                           
  ** qui depasse INT_MAX (2 147 483 647) si la multiplication se fait en int 32 bits.                                                                                                                              
  */
 
void	ms_to_timespec(long long delta_ms, struct timespec *ts)
{
	long long	abs_ms;

	abs_ms = now_ms() + delta_ms;
	ts->tv_sec = abs_ms / 1000;
	ts->tv_nsec = (abs_ms % 1000) * 1000000LL;
}