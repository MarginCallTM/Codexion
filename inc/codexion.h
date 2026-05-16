/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 11:08:59 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 12:36:35 by acombier         ###   ########.fr       */
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


// Saves the simulation settings
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

// Represents the state of the simulation
typedef enum e_state
{
	STATE_COMPILING,
	STATE_DEBUGGING,
	STATE_REFACTORING
}	t_state;


typedef struct s_pqueue		t_pqueue;
typedef struct s_dongle		t_dongle;
typedef struct s_coder		t_coder;
typedef struct s_sim	t_sim;

// Represents a shared resource protected by a mutex
typedef struct s_dongle
{
	int		id;
	
	pthread_mutex_t mutex;
	
	int		taken;
	int		stop_requested;
	long long	released_at_ms;
	
	t_pqueue	*waiters;
	pthread_cond_t	cond;

}	t_dongle;

// Represents a coder/thread
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

// Represents the entire simulation
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

	pthread_t		monitor;

}	t_sim;


// Represents a coder in a priority queue
typedef struct s_pqnode
{
	int		coder_id;
	long long key;
	long long tiebreak;			
}	t_pqnode;

// Priority queue for managing access to dongles
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

/* === Phase 5 (logs serialises) === */

void	log_event(t_sim *sim, int coder_id, const char *msg);
void	log_taken_dongle(t_sim *sim, int coder_id);
void	log_compiling(t_sim *sim, int coder_id);
void	log_debugging(t_sim *sim, int coder_id);
void	log_refactoring(t_sim *sim, int coder_id);
void	log_burned_out(t_sim *sim, int coder_id);

/* === Phase 6 (helpers temps, debut) === */

long long now_ms(void);
long long elapsed_ms(t_sim *sim);
void	precise_sleep_ms(long long ms, t_sim *sim);
void	ms_to_timespec(long long delta_ms, struct timespec *ts);

/* === Phase 7 (dongles) === */                                                                                                                                                                                  
/* dongle_init / destroy / request / release : publiques */
int		dongle_init(t_dongle *d, int id);
void	dongle_destroy(t_dongle *d);
void	dongle_request(t_coder *coder, t_dongle *d);
void	dongle_release(t_dongle *d);


/* helpers internes (dans dongle_utils.c, non-static car fichier separe) */
long long	dongle_compute_key(t_coder *coder, t_sim *sim);
int		dongle_can_take(t_dongle *d, int coder_id, long long cooldown);
void	dongle_wait_loop(t_dongle *d, t_coder *coder, t_sim *sim);

/* === Phase 8 (coder threads) === */

void	*coder_routine(void *arg);
void	coder_take_dongles(t_coder *coder);

void	coder_release_dongles(t_coder *coder);
void	coder_do_cycle(t_coder *coder);
int		coder_should_continue(t_coder *coder);

/* === Phase 9 (monitor thread) === */

void	*monitor_routine(void *arg);
int		monitor_check_burnout(t_sim *sim);
int		monitor_check_end(t_sim *sim);
void	monitor_trigger_burnout(t_sim *sim, int coder_id);
void	monitor_signal_all_dongles(t_sim *sim);

/* === Phase 10 (sim lifecycle + main) === */

int		sim_init(t_sim *sim, const t_config *cfg);
void	sim_destroy(t_sim *sim);

int		sim_init_mutexes(t_sim *sim);
int		sim_init_dongles(t_sim *sim);
int		sim_init_coders(t_sim *sim);

int		sim_launch(t_sim *sim);
void	sim_join(t_sim *sim);
void	sim_emergency_stop(t_sim *sim, int created);

int		parse_args(int argc, char **argv, t_config *cfg);


#endif
