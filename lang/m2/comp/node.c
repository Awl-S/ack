/* N O D E   O F   A N   A B S T R A C T   P A R S E T R E E */

#ifndef NORCSID
static char *RcsId = "$Header$";
#endif

#include	"debug.h"

#include	<em_label.h>
#include	<em_arith.h>
#include	<alloc.h>
#include	<system.h>

#include	"def.h"
#include	"type.h"
#include	"LLlex.h"
#include	"node.h"

struct node *h_node;		/* header of free list */
#ifdef DEBUG
int	cnt_node;		/* count number of allocated ones */
#endif

struct node *
MkNode(class, left, right, token)
	struct node *left, *right;
	struct token *token;
{
	/*	Create a node and initialize it with the given parameters
	*/
	register struct node *nd = new_node();

	nd->nd_left = left;
	nd->nd_right = right;
	nd->nd_token = *token;
	nd->nd_class = class;
	nd->nd_type = error_type;
	DO_DEBUG(4,(debug("Create node:"), PrNode(nd)));
	return nd;
}

struct node *
MkLeaf(class, token)
	struct token *token;
{
	register struct node *nd = new_node();

	nd->nd_left = nd->nd_right = 0;
	nd->nd_token = *token;
	nd->nd_type = error_type;
	nd->nd_class = class;
	return nd;
}

FreeNode(nd)
	register struct node *nd;
{
	/*	Put nodes that are no longer needed back onto the free
		list
	*/
	if (!nd) return;
	FreeNode(nd->nd_left);
	FreeNode(nd->nd_right);
	free_node(nd);
}

NodeCrash(expp)
	struct node *expp;
{
	crash("Illegal node %d", expp->nd_class);
}

#ifdef DEBUG

extern char *symbol2str();

STATIC
printnode(nd)
	register struct node *nd;
{
	fprint(STDERR, "(");
	if (nd) {
		printnode(nd->nd_left);
		fprint(STDERR, " %s ", symbol2str(nd->nd_symb));
		printnode(nd->nd_right);
	}
	fprint(STDERR, ")");
}

PrNode(nd)
	struct node *nd;
{
	printnode(nd);
	fprint(STDERR, "\n");
}
#endif DEBUG
