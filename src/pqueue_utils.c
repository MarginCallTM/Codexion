/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pqueue_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 14:14:26 by acombier          #+#    #+#             */
/*   Updated: 2026/05/16 14:13:28 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

// Check if coder A is priority on coder B
int	pq_less(t_pqnode a, t_pqnode b)
{
	if (a.key != b.key)
		return (a.key < b.key);
	return (a.tiebreak < b.tiebreak);
}

void	sift_up(t_pqueue *q, size_t i)
{
	size_t		parent;
	t_pqnode	tmp;

	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (!pq_less(q->data[i], q->data[parent]))
			break ;
		tmp = q->data[i];
		q->data[i] = q->data[parent];
		q->data[parent] = tmp;
		i = parent;
	}
}

void	sift_down(t_pqueue *q, size_t i)
{
	size_t		l;
	size_t		best;
	t_pqnode	tmp;

	while (1)
	{
		l = 2 * i + 1;
		best = i;
		if (l < q->size && pq_less(q->data[l], q->data[best]))
			best = l;
		if (l + 1 < q->size && pq_less(q->data[l + 1], q->data[best]))
			best = l + 1;
		if (best == i)
			break ;
		tmp = q->data[i];
		q->data[i] = q->data[best];
		q->data[best] = tmp;
		i = best;
	}
}

// Enlarge the tab by 2
int	pq_grow(t_pqueue *q)
{
	t_pqnode	*new_data;
	size_t		new_cap;
	size_t		i;

	new_cap = q->capacity * 2;
	new_data = malloc(sizeof(*new_data) * new_cap);
	if (!new_data)
		return (-1);
	i = 0;
	while (i < q->size)
	{
		new_data[i] = q->data[i];
		i++;
	}
	free(q->data);
	q->data = new_data;
	q->capacity = new_cap;
	return (0);
}

// Remove a specific coder in the queue
int	pq_remove(t_pqueue *q, int coder_id)
{
	size_t	i;

	i = 0;
	while (i < q->size)
	{
		if (q->data[i].coder_id == coder_id)
		{
			q->size--;
			if (i < q->size)
			{
				q->data[i] = q->data[q->size];
				if (i > 0 && pq_less(q->data[i], q->data[(i - 1) / 2]))
					sift_up(q, i);
				else
					sift_down(q, i);
			}
			return (0);
		}
		i++;
	}
	return (-1);
}
