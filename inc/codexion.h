/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 11:08:59 by acombier          #+#    #+#             */
/*   Updated: 2026/04/28 16:03:02 by acombier         ###   ########.fr       */
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

	long long		ticket_counter;

	pthread_t		monitor;

}	t_sim;

  /*                                                                                                                                                                                                               
  ** Noeud du heap.
  **   coder_id : qui attend dans la file
  **   key      : cle de tri (ticket FIFO ou deadline EDF, calculee par                                                                                                                                            
  **              l appelant ; le heap ne sait pas lequel des deux c est)                                                                                                                                          
  **   tiebreak : departage les keys egales pour rester deterministe                                                                                                                                               
  **              (on prend coder_id, qui est unique par construction)                                                                                                                                             
  */ 

typedef struct s_pqnode
{
	int		coder_id;
	long long key;
	long long tiebreak;			
}	t_pqnode;


struct s_pqueue
{
	t_pqnode	*data;
	size_t		size;
	size_t		capacity;
};

t_pqueue	*pq_init(size_t initial_capacity);
void	pq_destroy(t_pqueue *q);
int		pq_push(t_pqueue *q, int coder_id, long long key);
int		pq_peek(const t_pqueue *q, t_pqnode *out);
int		pq_pop(t_pqueue *q, t_pqnode *out);
int		pq_remove(t_pqueue *q, int coder_id);

int		pq_less(t_pqnode a, t_pqnode b);
void	sift_up(t_pqueue *q, size_t i);
void	sift_down(t_pqueue *q, size_t i);
int		pq_grow(t_pqueue *q);


/* === Phase 6 (helpers temps, debut) === */

long long now_ms(void);
long long elapsed_ms(t_sim *sim);

/* === Phase 5 (logs serialises) === */

void	log_event(t_sim *sim, int coder_id, const char *msg);
void	log_taken_dongle(t_sim *sim, int coder_id);
void	log_compiling(t_sim *sim, int coder_id);
void	log_debugging(t_sim *sim, int coder_id);
void	log_refactoring(t_sim *sim, int coder_id);
void	log_burned_out(t_sim *sim, int coder_id);



int		parse_args(int argc, char **argv, t_config *cfg);


#endif
