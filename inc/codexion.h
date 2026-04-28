/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 11:08:59 by acombier          #+#    #+#             */
/*   Updated: 2026/04/28 12:26:31 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/time.h>
# include <unistd.h>
# include <errno.h>

typedef enum e_scheduler
{
	SCHED_FIFO_,
	SCHED_EDF
} t_scheduler;

typedef struct s_config
{
	int		number_of_coders;
	long long	time_to_burnout;
	long long	time_to_compile;
	long long	time_to_debug;
	long long	time_to_refactor;
	int		number_of_compiles_required;
	long long	dongle_cooldown;
	t_scheduler		scheduler;
}	t_config;


 /*
  ** Etats d un coder. On utilise un enum (pas des #define) pour beneficier                                                     
  ** des warnings -Wswitch sur les switch incomplets et pour le typage fort.                                                    
  ** Prefixe STATE_ : evite toute collision avec d eventuelles macros systeme                                                   
  ** ou avec les constantes pthread (PTHREAD_*, etc.).                                                                          
  */ 

typedef enum e_state
{
	STATE_COMPILING,
	STATE_DEBUGGING,
	STATE_REFACTORING
}	t_state;

  /*              
  ** Forward declarations.
  ** Pourquoi ? Les 4 types ci-dessous se referencent mutuellement :
  **   - t_dongle contient un t_pqueue * (defini en Phase 4)                                                                    
  **   - t_coder  contient un t_sim *                                                                                           
  **   - t_sim    contient un t_coder * et un t_dongle *                                                                        
  ** Le compilateur a besoin de connaitre les NOMS de type avant de voir les                                                    
  ** corps qui les utilisent. Declarer "typedef struct s_X t_X;" sans corps                                                     
  ** suffit pour ecrire "t_X *" plus loin (un pointeur a une taille connue,                                                     
  ** independante de ce qu il pointe).                                                                                          
  */                                                                                                                            

typedef struct s_pqueue		t_pqueue;
typedef struct s_dongle		t_dongle;
typedef struct s_coder		t_coder;
typedef struct s_sim	t_sim;

/*              
  ** Un dongle = la ressource partagee qu un coder doit acquerir pour compiler.
  ** Equivalent de la "fourchette" dans le probleme des philosophes.                                                            
  ** Tous les champs sont proteges par "mutex" (sauf "id" qui est immuable
  ** apres init et peut donc etre lu sans lock).                                                                                
  */

typedef struct s_dongle
{
	int		id;
	
	pthread_mutex_t mutex;
	
	int		taken;
	long long	released_at_ms;
	
	t_pqueue	*waiters;
	pthread_cond_t	cond;

}	t_dongle;


typedef struct s_coder
{
	int		id;
	pthread_t	thread;
	t_state		state;
	long long	last_compile_start_ms;
	int			compiles_done;
	t_dongle	*left;
	t_dongle	*right;
	t_sim		*sim;
		
}	t_coder;


typedef struct s_sim
{
	t_config		config;
	t_coder			*coders;
	t_dongle		*dongles;
	long long		start_ms;

	int		stop;

	pthread_mutex_t stop_mutex;
	pthread_mutex_t	log_mutex;
	pthread_mutex_t	coder_state_mutex;

	long long		ticker_counter;

	pthread_t		monitor;

}	t_sim;

int		parse_args(int argc, char **argv, t_config *cfg);


#endif
