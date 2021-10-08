#include "proc.h"
#include "misc.h"
#include "limits.h"
_buffer	*_proc_buf=NULL;
//______________________________________________________________________________
__weak void Watchdog(void) {
}
//______________________________________________________________________________
__weak 	int	_print(const char *format, ...) {
			int ret=0;
			if(stdout->io) {
				char buf[128];	
				va_list	aptr;
				va_start(aptr, format);
				ret = vsnprintf(buf, 128, format, aptr);
				va_end(aptr);
				for(char *p=buf; *p; ++p)
					while(fputc(*p,stdout)==EOF)
						_wait(2);
			}
			return(ret);
}
//______________________________________________________________________________
_proc	*_proc_add(void *f,void *arg,char *name, int dt) {
_proc	*p=malloc(sizeof(_proc));
			if(p != NULL) {
				p->f=(func *)f;
				p->arg=arg;
				p->name=name;
				p->t=HAL_GetTick();
				p->dt=dt;
				if(!_proc_buf) {
					_proc_buf=_buffer_init(_PROC_BUFFER_SIZE*sizeof(_proc));				
				}
				_buffer_push(_proc_buf,&p,sizeof(_proc *));
			}
			return p;
}
//______________________________________________________________________________
_proc	*_proc_find(void  *f,void *arg) {
_proc	*p,*q=NULL;
int		i=_buffer_count(_proc_buf)/sizeof(_proc *);
			while(i--) {
				_buffer_pull(_proc_buf,&p,sizeof(_proc *));
				if(f == p->f && (arg == p->arg || !arg))
					q=p;
				_buffer_push(_proc_buf,&p,sizeof(_proc *));
			}
			return q;
}
//______________________________________________________________________________
void	_proc_list(void) {
_proc	*p;	
int		i	=_buffer_count(_proc_buf)/sizeof(_proc *);
			_print("...\r\n");
			while(i--) {
				_buffer_pull(_proc_buf,&p,sizeof(_proc *));
				_print("%08X,%08X,%s,%d\r\n",(int)p->f,(int)p->arg,p->name,p->to);
				_buffer_push(_proc_buf,&p,sizeof(_proc *));
			}
}
//______________________________________________________________________________
void	*_proc_loop(void) {
			_proc	*p=NULL;
			if(_proc_buf && _buffer_pull(_proc_buf,&p,sizeof(_proc *)) && p) {
				if(HAL_GetTick() >= p->t) {
					p->to = HAL_GetTick() - p->t;
					p->f(p->arg);
					p->t = HAL_GetTick() + p->dt;
				}
				_buffer_push(_proc_buf,&p,sizeof(_proc *));
			}
			return p;
}
//______________________________________________________________________________
void	_wait(int t) {
uint32_t	tout = HAL_GetTick() + t;
			while(HAL_GetTick() < tout) {
				_proc_loop();
				Watchdog();
			}
}
