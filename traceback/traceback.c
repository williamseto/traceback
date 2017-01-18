/** @file traceback.c
 *  @brief The traceback function
 *
 *  This file contains the traceback function for the traceback library
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @bug Unimplemented
 */

#include "traceback_internal.h"
#include "traceutils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>


int segvflag = 0;
sigjmp_buf segvjmp;


static void handler(int sig, siginfo_t *si, void *unused)
{
    //printf("Got SIGSEGV\n");
    segvflag = 1;
    siglongjmp(segvjmp, -1);
}

//check isprint for each character in the string
int isprint_str(char* teststr)
{
    int i;

    if (sigsetjmp(segvjmp, 0) == -1) {
        return 0;
    } 
    for (i = 0; i < strlen(teststr); i++) {
        if (!isprint(teststr[i]))
            return 0;
    }
    return 1;
}
int print_func_args(char* c_ebp, int idx, FILE* fp)
{
    int i;
    char tmpchar;
    char* tmpstr;
    char** tmpstrarr;
    int k;


    fprintf(fp, "Function %s(", functions[idx].name);

    for (i = 0; i < ARGS_MAX_NUM; i++) {

        //no more args
        if (strlen(functions[idx].args[i].name) == 0)
            break;

        if (i > 0)
            fprintf(fp, ", ");

        if (sigsetjmp(segvjmp, 0) == -1) {
            //this case covers if we can't dereference the offset
            //of ebp for whatever reason. note, this is different
            //from the case of an unprintable string, this would
            //be the case that the pointer to our string is bogus,
            //vs an unterminated string for example
            fprintf(fp, "%p", c_ebp+functions[idx].args[i].offset);
            continue;
        }

        switch (functions[idx].args[i].type) {
            case TYPE_CHAR:

                fprintf(fp, "char %s=", functions[idx].args[i].name);

                tmpchar = (c_ebp+functions[idx].args[i].offset)[0];
                if (isprint(tmpchar))
                    fprintf(fp, "'%c'", tmpchar);
                else
                    fprintf(fp, "'\\%o'", tmpchar);
                break;
            case TYPE_INT:

                fprintf(fp, "int %s=", functions[idx].args[i].name);
                fprintf(fp, "%d",
                //            *((int*)(0xdeadbeef)));
                            *((int*)(c_ebp+functions[idx].args[i].offset)));
                break;
            case TYPE_FLOAT:
                fprintf(fp, "float %s=", functions[idx].args[i].name);
                fprintf(fp, "%f",
                            *((float*)(c_ebp+functions[idx].args[i].offset)));
                break;
            case TYPE_DOUBLE:
                fprintf(fp, "double %s=", functions[idx].args[i].name);
                fprintf(fp, "%g",
                            *((double*)(c_ebp+functions[idx].args[i].offset)));
                break;
            case TYPE_STRING:

                fprintf(fp, "char *%s=", functions[idx].args[i].name);
                tmpstr = *((char**)(c_ebp+functions[idx].args[i].offset));

                if (isprint_str(tmpstr)) {
                    if (strlen(tmpstr) > 25)
                        fprintf(fp, "\"%*.*s...\"", 0, 25, tmpstr);
                    else
                        fprintf(fp, "\"%*.*s\"", 0, 25, tmpstr);
                } else {
                    fprintf(fp, "%p", tmpstr);
                }

                break;
            case TYPE_STRING_ARRAY:

                fprintf(fp, "char **%s=", functions[idx].args[i].name);
                tmpstrarr = *((char***)(c_ebp+functions[idx].args[i].offset));


                fprintf(fp, "{");
                for (k = 0; tmpstrarr[k]; k++) {
                    tmpstr = tmpstrarr[k];

                    if (k > 0)
                        fprintf(fp, ", ");

                    if (k == 3) {
                        fprintf(fp, "...");
                        break;
                    }

                    if (isprint_str(tmpstr)) {
                        if (strlen(tmpstr) > 25)
                            fprintf(fp, "\"%*.*s...\"", 0, 25, tmpstr);
                        else
                            fprintf(fp, "\"%*.*s\"", 0, 25, tmpstr);
                    } else {
                        fprintf(fp, "%p", tmpstr);
                    }
                }
                fprintf(fp, "}");
                break;
            case TYPE_VOIDSTAR:
                fprintf(fp, "void *%s=", functions[idx].args[i].name);
                fprintf(fp, "0v%lx",
                        (long unsigned int)
                        *(void**)(c_ebp+functions[idx].args[i].offset));

                break;
            case TYPE_UNKNOWN:
                fprintf(fp, "UNKNOWN %s=", functions[idx].args[i].name);
                fprintf(fp, "%p",
                            *(void**)(c_ebp+functions[idx].args[i].offset));
                break;
        }

    }

    if (i == 0)
        fprintf(fp, "void");

    fprintf(fp, "), in\n");

    return 0;
}

int get_func_idx(void* oldeip)
{
    char* eip = (char*) oldeip; //cast to char pointer so we can do arithmetic
    int idx = 0;
    int k = 0;
    for (k = 0; k < MAX_FUNCTION_SIZE_BYTES; k++) {
        for (idx = 0; idx < FUNCTS_MAX_NUM; idx++) {
            if (strlen(functions[idx].name) == 0) //end of function list
                break;
            if (functions[idx].addr == eip)
                return idx;
        }
        eip--;
    }
    return -1;
}

void traceback(FILE *fp)
{
	/* the following just makes a sample access to "functions" array. 
	 * note if "functions" is not referenced somewhere outside the 
	 * file that it's declared in, symtabgen won't be able to find
	 * the symbol. So be sure to always do something with functions */


    void* tmpebp = currframeptr();
    void* tmpretaddr = NULL;

    int tmpfuncidx = -1;


    struct sigaction sa;


    //undo any blocking of our signals
    sigset_t unblockset;
    sigaddset(&unblockset, SIGSEGV);
    sigaddset(&unblockset, SIGINT);
    sigprocmask(SIG_UNBLOCK, &unblockset, NULL);
    sa.sa_flags = SA_ONSTACK | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    sigaction (SIGSEGV, &sa, NULL);




    while (1) {

        if (sigsetjmp(segvjmp, 0) == -1) {
            fprintf(fp, "FATAL: bad stack\n");
            return;
        } 
        tmpretaddr = callretaddr(tmpebp);
        tmpfuncidx = get_func_idx(tmpretaddr);

        tmpebp = prevebp(tmpebp);

        if (tmpfuncidx > -1) {
            if (strcmp(functions[tmpfuncidx].name, "__libc_start_main") == 0) {
                fprintf(fp, "Function __libc_start_main(void) \n");
                return;
            }

            print_func_args(tmpebp, tmpfuncidx, fp);

        } else {
            fprintf(fp, "Function %p(...), in\n", tmpretaddr);
        }

    }

}


