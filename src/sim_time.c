/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_time.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 16:03:43 by acombier          #+#    #+#             */
/*   Updated: 2026/04/28 16:07:51 by acombier         ###   ########.fr       */
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