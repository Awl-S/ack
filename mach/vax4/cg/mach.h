/* $Header$ */
#define ex_ap(x)	fprintf(codefile,".globl\t%s\n",x)
#define in_ap(x)	/* nothing */

#define newilb(x)	fprintf(codefile,"%s:\n",x)
#define newdlb(x)	fprintf(codefile,"%s:\n",x)
#define newplb(x)	fprintf(codefile,".align 1\n%s:\n",x)
#define	dlbdlb(s1,s2)	fprintf(codefile,".set %s, %s\n",s1,s2)
#define	newlbss(l,x)	fprintf(codefile,".lcomm\t%s,%d\n",l,x);

#define cst_fmt		"%ld"
#define	off_fmt		"%ld"
#define ilb_fmt		"L%xL%x"
#define dlb_fmt		"_%d"
#define	hol_fmt		"hol%d"

#define fmt_id(fr,to)	sprintf(to,"_%s",fr)

#define hol_off		"%ld+hol%d"

#define con_cst(w)	fprintf(codefile,".long\t%ld\n",w)
#define con_ilb(x)	fprintf(codefile,".long\t%s\n",x)
#define con_dlb(x)	fprintf(codefile,".long\t%s\n",x)

#define BSS_INIT	0
