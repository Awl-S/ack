#include "tables.h"
#ifdef USE_TES
#include "types.h"
#include "param.h"
#include "label.h"

static label_p label_list = (label_p)0;
extern char *myalloc();

add_label(num, height, flth)
{
	register label_p lbl = (label_p)0;

	if (height <= 0) return;
	if (flth != TRUE && flth != FALSE)
	    fatal("incorrect value for fallthrough");

	lbl = (label_p) myalloc(sizeof(label_t));
	lbl->lb_next = label_list;
	lbl->lb_number = num;
	lbl->lb_height = height;
	lbl->lb_fallthrough = flth;
	label_list = lbl;
}

label_p get_label(num)
register word num;
{
	register label_p tmp = label_list;

	while (tmp != (label_p)0) {
		if (tmp->lb_number == num) return tmp;
		tmp = tmp->lb_next;
	}
	return (label_p)0;
}

kill_labels()
{
	label_p tmp;

	while((tmp = label_list) != (label_p)0) {
	    label_list = label_list->lb_next;
	    myfree((char *)tmp);
	}
}
#endif
