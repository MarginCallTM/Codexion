/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pqueue.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 12:31:20 by acombier          #+#    #+#             */
/*   Updated: 2026/04/28 13:10:57 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

  /*              
  ** initial_capacity = 0 -> on prend 8 par defaut (heap auto-extensible).
  ** L appelant peut aussi passer config.number_of_coders si on veut eviter                                                                                                                                        
  ** tout realloc (un dongle a au plus N waiters).                                                                                                                                                                 
  */

t_pqueue	*pq_init(size_t initial_capacity)
{
	t_pqueue	*q;
	
	if(initial_capacity == 0)
		initial_capacity = 8;
	q = malloc(sizeof(*q));
	if(!q)
		return(NULL);
	q->data = malloc(sizeof(*q->data) * initial_capacity);
	if(!q->data)
		return(free(q), NULL);
	q->size = 0;
	q->capacity = initial_capacity;
	return(q);	
}

  /*
  ** Tolere q == NULL (idiome free()) pour simplifier les chemins de cleanup.
  */ 

void	pq_destroy(t_pqueue *q)
{
	if(!q)
		return;
	free(q->data);
	free(q);
}

 /*                                                                                                                                                                                                               
  ** O(log n). On ajoute en fin (preserve la propriete d arbre quasi-complet)
  ** puis on fait remonter l element a sa bonne place. Retourne -1 si la                                                                                                                                           
  ** reallocation a echoue.                                                                                                                                                                                        
  */ 
 
int		pq_push(t_pqueue *q, int coder_id, long long key)
{
	if(q->size == q->capacity && pq_grow(q) != 0)
		return (-1);
	q->data[q->size].coder_id = coder_id;
	q->data[q->size].key = key;
	q->data[q->size].tiebreak = coder_id;
	q->size++;
	sift_up(q, q->size - 1);
	return (0);
}

  /*
  ** Lecture seule : on ne modifie pas la file. Retourne -1 si vide pour
  ** distinguer "pas de tete" d une "tete avec coder_id == 0" (id valide).                                                                                                                                         
  ** *out non touche en cas d echec.                                                                                                                                                                               
  */  


int pq_peek(const t_pqueue *q, t_pqnode *out)
{
	if(q->size == 0)
		return (-1);
	*out = q->data[0];
	return (0);
}

  /*
  ** O(log n). On sauve la racine, on met le dernier element au sommet
  ** et on le fait redescendre. Le caller recupere la valeur via *out.                                                                                                                                             
  */ 

int pq_pop(t_pqueue *q, t_pqnode *out)
{
	if(q->size == 0)
		return (-1);
	*out = q->data[0];
	q->size--;
	if(q->size > 0)
	{
		q->data[0] = q->data[q->size];
		sift_down(q, 0);
	}
	return (0);
}