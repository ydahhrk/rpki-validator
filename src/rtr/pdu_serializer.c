#include "pdu_serializer.h"

#include "primitive_writer.h"

static size_t
serialize_pdu_header(struct pdu_header *header, u_int16_t union_value,
    char *buf)
{
	char *ptr;

	ptr = buf;
	ptr = write_int8(ptr, header->protocol_version);
	ptr = write_int8(ptr, header->pdu_type);
	ptr = write_int16(ptr, union_value);
	ptr = write_int32(ptr, header->length);
	return ptr - buf;
}

size_t
serialize_serial_notify_pdu(struct serial_notify_pdu *pdu, char *buf)
{
	// FIXME Complete me!
	return 0;
}

size_t
serialize_cache_response_pdu(struct cache_response_pdu *pdu, char *buf)
{
	/* No payload to serialize */
	return serialize_pdu_header(&pdu->header, pdu->header.session_id, buf);
}

size_t
serialize_ipv4_prefix_pdu(struct ipv4_prefix_pdu *pdu, char *buf)
{
	// FIXME Complete me!
	return 0;
}

size_t
serialize_ipv6_prefix_pdu(struct ipv6_prefix_pdu *pdu, char *buf)
{
	// FIXME Complete me!
	return 0;
}

size_t
serialize_end_of_data_pdu(struct end_of_data_pdu *pdu, char *buf)
{
	size_t head_size;
	char *ptr;

	head_size = serialize_pdu_header(&pdu->header, pdu->header.session_id, buf);

	ptr = buf + head_size;
	ptr = write_int32(ptr, pdu->serial_number);
	if (pdu->header.protocol_version == RTR_V1) {
		ptr = write_int32(ptr, pdu->refresh_interval);
		ptr = write_int32(ptr, pdu->retry_interval);
		ptr = write_int32(ptr, pdu->expire_interval);
	}

	return ptr - buf;
}

size_t
serialize_cache_reset_pdu(struct cache_reset_pdu *pdu, char *buf)
{
	// FIXME Complete me!
	return 0;
}

size_t
serialize_error_report_pdu(struct error_report_pdu *pdu, char *buf)
{
	// FIXME Complete me!
	return 0;
}
