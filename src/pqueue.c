/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pqueue.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 12:31:20 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 14:05:57 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Create an priority queue
t_pqueue	*pq_init(size_t initial_capacity)
{
	t_pqueue	*q;

	if (initial_capacity == 0)
		initial_capacity = 8;
	q = malloc(sizeof(*q));
	if (!q)
		return (NULL);
	q->data = malloc(sizeof(*q->data) * initial_capacity);
	if (!q->data)
		return (free(q), NULL);
	q->size = 0;
	q->capacity = initial_capacity;
	return (q);
}

// free and destroy the queue
void	pq_destroy(t_pqueue *q)
{
	if (!q)
		return ;
	free(q->data);
	free(q);
}

// Add a coder in the queue
int	pq_push(t_pqueue *q, int coder_id, long long key)
{
	if (q->size == q->capacity && pq_grow(q) != 0)
		return (-1);
	q->data[q->size].coder_id = coder_id;
	q->data[q->size].key = key;
	q->data[q->size].tiebreak = coder_id;
	q->size++;
	sift_up(q, q->size - 1);
	return (0);
}

// Check the most prioritary in the queue
int	pq_peek(const t_pqueue *q, t_pqnode *out)
{
	if (q->size == 0)
		return (-1);
	*out = q->data[0];
	return (0);
}

// Remove the priority coders in the queue
int	pq_pop(t_pqueue *q, t_pqnode *out)
{
	if (q->size == 0)
		return (-1);
	*out = q->data[0];
	q->size--;
	if (q->size > 0)
	{
		q->data[0] = q->data[q->size];
		sift_down(q, 0);
	}
	return (0);
}
