/* 
 *   lib/procop.c
 *   Process control
 *   
 *   Copyright (C) 2005-2008 Ville H. Tuulos
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <dub.h>

int exec_and_read(const char *proc, int *proc_stdin, pid_t *child,
                        char *const envp[])
{
        int fds[2];

        if (pipe(fds))
                dub_sysdie("Couldn't open pipe (exec_and_read)");
        
        signal(SIGCLD, SIG_IGN);
        
        if (!(*child = fork())){
                
                if (proc_stdin){
                        dup2(*proc_stdin, 0);
                        close(*proc_stdin);
                }
                
                close(fds[0]);
                
                dup2(fds[1], 1);
                close(fds[1]);
                                
                if (envp){
                        if (execle(proc, proc, NULL, envp))
                                dub_sysdie("Exec failed");
                }else
                        if (execlp(proc, proc, NULL))
                                dub_sysdie("Exec failed");
                
        }else{
                if (proc_stdin)
                        close(*proc_stdin);
                
                close(fds[1]);
                return fds[0];
        }
        
        /* never happens */
        return 0;
}

int exec_and_write(const char *proc, int *proc_stdout, pid_t *child,
                        char *const envp[])
{
        int fds[2];

        if (pipe(fds))
                dub_sysdie("Couldn't open pipe (exec_and_write)");
       
        signal(SIGCLD, SIG_IGN);
        
        if (!(*child = fork())){
                
                if (proc_stdout){
                        dup2(*proc_stdout, 1);
                        close(*proc_stdout);
                }
                
                close(fds[1]);
                
                dup2(fds[0], 0);
                close(fds[0]);
                
                if (envp){
                        if (execle(proc, proc, NULL, envp))
                                dub_sysdie("Exec failed");
                }else
                        if (execlp(proc, proc, NULL))
                                dub_sysdie("Exec failed");
                
        }else{
                if (proc_stdout)
                        close(*proc_stdout);
                
                close(fds[0]);
                return fds[1];
        }

        /* never happens */
        return 0;
}

int exec_and_rw(const char *proc, int *proc_stdout, pid_t *child,
                char *const envp[])
{
        int fds1[2];
        int fds2[2];

        if (pipe(fds1) || pipe(fds2))
                dub_sysdie("Couldn't open pipe (exec_and_rw)");
       
        signal(SIGCLD, SIG_IGN);
        
        if (!(*child = fork())){
                
                close(fds1[1]);
                close(fds2[0]);
                
                dup2(fds1[0], 0);
                close(fds1[0]);
                
                dup2(fds2[1], 1);
                close(fds2[1]);
                
                if (envp){
                        if (execle(proc, proc, NULL, envp))
                                dub_sysdie("Exec failed");
                }else
                        if (execlp(proc, proc, NULL))
                                dub_sysdie("Exec failed");
                
        }else{
                close(fds1[0]);
                close(fds2[1]);

                *proc_stdout = fds2[0];
                
                return fds1[1];
        }

        /* never happens */
        return 0;

}

#if TEST
int main(int argc, char **argv)
{
        static char str[] = "2+2\nquit\n";
        char buf[1024];
        
        int bc_out;
        pid_t child;
        
        int bc_in = exec_and_rw("bc", &bc_out, &child, NULL);
        
        write(bc_in, str, sizeof(str));

        while (read(bc_out, buf, 1024) > 0)
                printf("%s", buf);

        printf("\n");
        
}
#endif
