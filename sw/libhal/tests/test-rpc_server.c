#include <hal.h>

#define interrupt 0

static uint8_t inbuf[HAL_RPC_MAX_PKT_SIZE], outbuf[HAL_RPC_MAX_PKT_SIZE];

static void hal_rpc_server_main(void)
{
    size_t ilen, olen;
    void *opaque;
    hal_error_t ret;

    while (!interrupt) {
        ilen = sizeof(inbuf);
        ret = hal_rpc_recvfrom(inbuf, &ilen, &opaque);
        if (ret == HAL_OK) {
            olen = sizeof(outbuf);
            if (hal_rpc_server_dispatch(inbuf, ilen, outbuf, &olen) == HAL_OK)
                hal_rpc_sendto(outbuf, olen, opaque);
        }
    }
}

int main (int argc, char *argv[])
{
    if (hal_rpc_server_init() != HAL_OK)
	return 1;

    hal_rpc_server_main();
    hal_rpc_server_close();
    return 0;
}
