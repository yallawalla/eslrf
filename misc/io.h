#ifndef _IO_H
#define _IO_H

#ifdef __cplusplus
 extern "C" {
#endif

#include		<string.h>
#include		<stdlib.h>
#include		<stdarg.h>
#include		<stdio.h>
//______________________________________________________________________________________
typedef struct _buffer
{
	char	*_buf,
				*_push,
				*_pull;
	int		size;
} _buffer;	
//______________________________________________________________________________________
typedef	struct esc {
	unsigned int	seq, timeout;
} esc;
//______________________________________________________________________________________
typedef struct _io
{
	_buffer	*rx,*tx,*gets;
	int		(*get)(struct _buffer *),
				(*put)(struct _buffer *, int);
	void	*huart;
	esc		*esc;
} _io;
//______________________________________________________________________________________
_buffer	*_buffer_init(int),
				*_buffer_close(_buffer *);
_io			*_io_init(int, int),
				*_io_close(_io *),
				*_stdio(_io	*);

int			_buffer_push(_buffer *, void *,int),
				_buffer_put(_buffer *, void *,int),
				_buffer_pull(_buffer *, void *,int),
				_buffer_count(_buffer *);
				
int			putch(int),
				getch(void),
				ungetch(int),
				ungets(char *);
int			__get (_buffer *);
int			__put (_buffer *, int);
//______________________________________________________________________________________
void		Watchdog(void);
void		Watchdog_init(int);
//______________________________________________________________________________________
struct	__FILE
{ 
				_io		*io;
};
//______________________________________________________________________________________
int		_print(const char *format, ...);
int		_printdec(int, int);
//int		f_printf(FIL *, const char *format, ...);
#ifdef	__cplusplus
}
#endif

#endif
