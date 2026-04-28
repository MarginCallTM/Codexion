/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pqueue_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acombier <acombier@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 14:14:26 by acombier          #+#    #+#             */
/*   Updated: 2026/04/28 15:12:50 by acombier         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

  /*              
  ** Comparateur strict du heap. a < b si a.key < b.key, et a egalite de key
  ** on departage avec tiebreak (= coder_id, donc unique). Pourquoi strict :                                                                                                                                       
  ** un comparateur "<=" peut creer des swaps infinis dans sift_down quand                                                                                                                                         
  ** deux elements ont la meme cle.                                                                                                                                                                                
  */  

int		pq_less(t_pqnode a, t_pqnode b)
{
	if(a.key != b.key)
		return(a.key < b.key);
	return(a.tiebreak < b.tiebreak);
}

/*                                                                                                                                                                                                               
  ** Fait remonter le noeud i tant qu il est plus petit que son parent.
  ** Parent de i = (i - 1) / 2. On s arrete des qu on atteint la racine                                                                                                                                            
  ** (i == 0) ou que la propriete de heap est restauree.                                                                                                                                                           
  */

void	sift_up(t_pqueue *q, size_t i)
{
	size_t	parent;
	t_pqnode	tmp;
	
	while(i > 0)
	{
		parent = (i - 1) / 2;
		if(!pq_less(q->data[i], q->data[parent]))
			break;
		tmp = q->data[i];
		q->data[i] = q->data[parent];
		q->data[parent] = tmp;
		i = parent;
	}
}

  /*
  ** Fait descendre le noeud i en l echangeant avec son plus petit fils
  ** tant qu une violation existe. Enfants de i = 2i+1 (gauche) et 2i+2 (droit).                                                                                                                                   
  ** "best" suit le candidat le plus petit parmi {i, gauche, droit}.                                                                                                                                               
  */

void	sift_down(t_pqueue *q, size_t i)
{
	size_t	l;
	size_t	best;
	t_pqnode	tmp;

	while(1)
	{
		l = 2 * i + 1;
		best = i;
		if(l < q->size && pq_less(q->data[l], q->data[best]))
			best = l;
		if(l + 1 < q->size && pq_less(q->data[l + 1], q->data[best]))
			best = l + 1;
		if(best == i)
			break;
		tmp = q->data[i];
		q->data[i] = q->data[best];
		q->data[best] = tmp;
		i = best;
	}
}

 /*
  ** Double la capacite. On evite realloc (parfois interdit selon les sujets
  ** 42, et pas indispensable ici) en allouant + recopiant + free. Retourne                                                                                                                                        
  ** -1 si malloc echoue, sans toucher au buffer existant.                                                                                                                                                         
  */ 

int		pq_grow(t_pqueue *q)
{
	t_pqnode	*new_data;
	size_t	new_cap;
	size_t	i;

	new_cap = q->capacity * 2;
	new_data = malloc(sizeof(*new_data) * new_cap);
	if(!new_data)
		return (-1);
	i = 0;
	while(i < q->size)
	{
		new_data[i] = q->data[i];
		i++;
	}
	free(q->data);
	q->data = new_data;
	q->capacity = new_cap;
	return (0);
}

  /*                                                                                                                                                                                                               
  ** Retire le noeud dont coder_id == id, ou qu il soit dans le heap.
  ** Strategie : recherche lineaire (O(n)), on swap avec le dernier element,                                                                                                                                       
  ** puis on restaure la propriete de heap. Subtilite : le nouveau noeud peut                                                                                                                                      
  ** etre PLUS PETIT que son parent (-> sift_up) OU PLUS GRAND que ses fils                                                                                                                                        
  ** (-> sift_down). On choisit selon la comparaison avec le parent ; un seul                                                                                                                                      
  ** des deux sera reellement utile. Retourne -1 si l id n est pas trouve.                                                                                                                                         
  */

int		pq_remove(t_pqueue *q, int coder_id)
{
	size_t	i;
	i = 0;

	while(i < q->size)
	{
		if(q->data[i].coder_id == coder_id)
		{
			q->size--;
			if(i < q->size)
			{
				q->data[i] = q->data[q->size];
				if(i > 0 && pq_less(q->data[i], q->data[(i - 1) / 2]))
					sift_up(q, i);
				else
					sift_down(q, i);
			}
			return(0);
		}
		i++;
	}
	return (-1);
}


