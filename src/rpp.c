#include "rpp.h"

#include <stdlib.h>
#include "array_list.h"
#include "log.h"
#include "thread_var.h"
#include "uri.h"
#include "object/certificate.h"
#include "object/crl.h"
#include "object/roa.h"

ARRAY_LIST(uris, struct rpki_uri)

/** A Repository Publication Point (RFC 6481), as described by some manifest. */
struct rpp {
	struct uris certs; /* Certificates */

	struct rpki_uri crl; /* Certificate Revocation List */
	bool crl_set;

	/* The Manifest is not needed for now. */

	struct uris roas; /* Route Origin Attestations */
};

struct foreach_args {
	struct rpp *pp;
	STACK_OF(X509_CRL) *crls;
};

struct rpp *
rpp_create(void)
{
	struct rpp *result;

	result = malloc(sizeof(struct rpp));
	if (result == NULL)
		goto fail1;

	if (uris_init(&result->certs) != 0)
		goto fail2;
	result->crl_set = false;
	if (uris_init(&result->roas) != 0)
		goto fail3;

	return result;

fail3:
	uris_cleanup(&result->certs, uri_cleanup);
fail2:
	free(result);
fail1:
	return NULL;
}

void
rpp_destroy(struct rpp *pp)
{
	uris_cleanup(&pp->certs, uri_cleanup);
	uris_cleanup(&pp->roas, uri_cleanup);
	free(pp);
}

int
rpp_add_cert(struct rpp *pp, struct rpki_uri *uri)
{
	return uris_add(&pp->certs, uri);
}

int
rpp_add_roa(struct rpp *pp, struct rpki_uri *uri)
{
	return uris_add(&pp->roas, uri);
}

int
rpp_add_crl(struct rpp *pp, struct rpki_uri *uri)
{
	/* rfc6481#section-2.2 */
	if (pp->crl_set)
		return pr_err("Repository Publication Point has more than one CRL.");

	pp->crl = *uri;
	pp->crl_set = true;
	pr_debug("Manifest CRL: %s", uri->global);
	return 0;
}

static int
add_crl_to_stack(struct rpp *pp, STACK_OF(X509_CRL) *crls)
{
	X509_CRL *crl;
	int error;
	int idx;

	pr_debug("MANIFEST.CRL: %s", pp->crl.global);
	fnstack_push(pp->crl.global);

	error = crl_load(&pp->crl, &crl);
	if (error)
		goto end;

	idx = sk_X509_CRL_push(crls, crl);
	if (idx <= 0) {
		error = crypto_err("Could not add CRL to a CRL stack");
		X509_CRL_free(crl);
		goto end;
	}

end:
	fnstack_pop();
	return error;
}

static int
traverse_ca_certs(struct rpki_uri const *uri, void *arg)
{
	struct foreach_args *args = arg;
	return certificate_traverse(args->pp, uri, args->crls, false);
}

static int
print_roa(struct rpki_uri const *uri, void *arg)
{
	struct foreach_args *args = arg;
	handle_roa(uri, args->pp, args->crls);
	return 0;
}

struct rpki_uri const *
rpp_get_crl(struct rpp const *pp)
{
	return pp->crl_set ? &pp->crl : NULL;
}

int
rpp_traverse(struct rpp *pp)
{
	/*
	 * TODO is the stack supposed to have only the CRLs of this layer,
	 * or all of them?
	 */
	struct foreach_args args;
	int error;

	args.pp = pp;
	args.crls = sk_X509_CRL_new_null();
	if (args.crls == NULL)
		return pr_enomem();
	error = add_crl_to_stack(pp, args.crls);
	if (error)
		goto end;

	/* Use CRL stack to validate certificates, and also traverse them. */
	error = uris_foreach(&pp->certs, traverse_ca_certs, &args);
	if (error)
		goto end;

	/* Use valid address ranges to print ROAs that match them. */
	error = uris_foreach(&pp->roas, print_roa, &args);

end:
	sk_X509_CRL_pop_free(args.crls, X509_CRL_free);
	return error;
}