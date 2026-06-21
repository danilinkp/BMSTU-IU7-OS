#include "calculator.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

typedef union
{
	struct CALCULATOR calculator_proc_1_arg;
} argument_t;

struct thread_data_t
{
	struct svc_req *rqstp;
	SVCXPRT *transp;
	argument_t argument;
};
typedef struct thread_data_t thread_data_t;
pthread_attr_t attr;
pthread_t thread;

static void *calculator_prog_worker(void *data_ptr)
{
	union {
		float calculator_proc_1_res;
	} result;
	bool_t retval;

	bool_t (*local)(char *, void *, struct svc_req *) = 
		(bool_t (*) (char *, void *,  struct svc_req *))calculator_proc_1_svc;

	thread_data_t *thread_data_ptr = (thread_data_t *)data_ptr;
	struct svc_req *rqstp = thread_data_ptr->rqstp;
	register SVCXPRT *transp = thread_data_ptr->transp;

	retval = (bool_t) (*local)((char *)&thread_data_ptr->argument, (void *)&result, rqstp);
	if (retval > 0 && !svc_sendreply(transp, (xdrproc_t) xdr_float, (char *)&result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) xdr_CALCULATOR, (caddr_t) &thread_data_ptr->argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		free(data_ptr);
		exit (1);
	}
	if (!calculator_prog_1_freeresult (transp, (xdrproc_t) xdr_float, (caddr_t) &result))
		fprintf (stderr, "%s", "unable to free results");

	free(data_ptr);
	return NULL;
}

static void
calculator_prog_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	xdrproc_t _xdr_argument, _xdr_result;
	bool_t (*local)(char *, void *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case CALCULATOR_PROC:
		_xdr_argument = (xdrproc_t) xdr_CALCULATOR;
		_xdr_result   = (xdrproc_t) xdr_float;
		local = (bool_t (*) (char *, void *,  struct svc_req *))calculator_proc_1_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	thread_data_t *thread_data_ptr = (thread_data_t *)malloc(sizeof(thread_data_t));
	if (thread_data_ptr == NULL)
	{
		perror("malloc");
		exit(1);
	}
	thread_data_ptr->rqstp = rqstp;
	thread_data_ptr->transp = transp;
	memset ((char *)&thread_data_ptr->argument, 0, sizeof (thread_data_ptr->argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &thread_data_ptr->argument)) {
		svcerr_decode (transp);
		free(thread_data_ptr);
		return;
	}
	pthread_attr_init(&attr);
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
	{
		perror("pthread_attr_setdetachstate");
		svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &thread_data_ptr->argument);
		free(thread_data_ptr);
		exit(1);
	}
	if (pthread_create(&thread, &attr, calculator_prog_worker, (void *)thread_data_ptr) != 0)
	{
		perror("pthread_create");
		svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &thread_data_ptr->argument);
		free(thread_data_ptr);
		exit(1);
	}
}

int
main (int argc, char **argv)
{
	register SVCXPRT *transp;
	pmap_unset (CALCULATOR_PROG, CALCULATOR_VER);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create udp service.");
		exit(1);
	}
	if (!svc_register(transp, CALCULATOR_PROG, CALCULATOR_VER, calculator_prog_1, IPPROTO_UDP)) {
		fprintf (stderr, "%s", "unable to register (CALCULATOR_PROG, CALCULATOR_VER, udp).");
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create tcp service.");
		exit(1);
	}
	if (!svc_register(transp, CALCULATOR_PROG, CALCULATOR_VER, calculator_prog_1, IPPROTO_TCP)) {
		fprintf (stderr, "%s", "unable to register (CALCULATOR_PROG, CALCULATOR_VER, tcp).");
		exit(1);
	}

	svc_run ();
	fprintf (stderr, "%s", "svc_run returned");
	exit (1);
	/* NOTREACHED */
}