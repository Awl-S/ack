/*   I N T E R M E D I A T E   C O D E
 *
 *    L O W   L E V E L   I / O   R O U T I N E S
 */


extern int	table1();		/* (  )
					 * Read an instruction from the
					 * Compact Assembly Language input
					 * file (in 'neutral state').
					 */
extern int	table2();		/* ( )
					 * Read an instruction argument.
					 */
extern int	table3();		/* ( int )
					 * Read 'Common Table' item.
					 */
extern short	get_int();		/* ( )				*/
extern offset	get_off();		/* ( )				*/
extern char	readchar();		/* ( )				*/
extern		file_init();		/* (FILE *f, short state, long length)
					 * Input file initialization. All
					 * following read operations will read
					 * from the given file f. Also checks
					 * the magic number and sets global
					 * variable 'linecount' to 0.
					 * If the state is ARCHIVE, length
					 * specifies the length of the module.
					 */
extern		arch_init();		/* (FILE *arch)
					 * Same as file_init,but opens an
					 * archive file. So it checks the
					 * magic number for archives.
					 */
