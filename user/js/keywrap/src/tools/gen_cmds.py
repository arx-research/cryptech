#!/usr/bin/env python3
#======================================================================
# gen_cmds.py
# -----------
#
# Script that generates test writes that we need.
#======================================================================

cleartext = "8af887c58dfbc38e e0423eefcc0e032d cc79dd116638ca65 ad75dca2a2459f13 934dbe61a62cb26d 8bbddbabf9bf52bb e137ef1d3e30eacf 0fe456ec808d6798 dc29fe54fa1f784a a3c11cf394050095 81d3f1d596843813 a6685e503fac8535 e0c06ecca8561b6a 1f22c578eefb6919 12be2e1667946101 ae8c3501e6c66eb1 7e14f2608c9ce6fb ab4a1597ed49ccb3 930b1060f98c97d8 dc4ce81e35279c4d 30d1bf86c9b919a3 ce4f0109e77929e5 8c4c3aeb5de1ec5e 0afa38ae896df912 1c72c255141f2f5c 9a51be5072547cf8 a3b067404e62f961 5a02479cf8c202e7 feb2e258314e0ebe 62878a5c4ecd4e9d f7dab2e1fa9a7b53 2c2169acedb7998d 5cd8a7118848ce7e e9fb2f68e28c2b27 9ddc064db70ad73c 6dbe10c5e1c56a70 9c1407f93a727cce 1075103a4009ae2f 7731b7d71756eee1 19b828ef4ed61eff 164935532a94fa8f e62dc2e22cf20f16 8ae65f4b6785286c 253f365f29453a47 9dc2824b8bdabd96 2da3b76ae9c8a720 155e158fe389c8cc 7fa6ad522c951b5c 236bf964b5b1bfb0 98a39835759b9540 4b72b17f7dbcda93 6177ae059269f41e cdac81a49f5bbfd2 e801392a043ef068 73550a67fcbc039f 0b5d30ce490baa97 9dbbaf9e53d45d7e 2dff26b2f7e6628d ed694217a39f454b 288e7906b79faf4a 407a7d207646f930 96a157f0d1dca05a 7f92e318fc1ff62c e2de7f129b187053"

ciphertext = "4501c1ecadc6b5e3 f1c23c29eca60890 5f9cabdd46e34a55 e1f7ac8308e75c90 3675982bda99173a 2ba57d2ccf2e01a0 2589f89dfd4b3c7f d229ec91c9d0c46e a5dee3c048cd4611 bfeadc9bf26daa1e 02cb72e222cf3dab 120dd1e8c2dd9bd5 8bbefa5d14526abd 1e8d2170a6ba8283 c243ec2fd5ef0703 0b1ef5f69f9620e4 b17a363934100588 7b9ffc7935335947 03e5dcae67bd0ce7 a3c98ca65815a4d0 67f27e6e66d6636c ebb789732566a52a c3970e14c37310dc 2fcee0e739a16291 029fd2b4d534e304 45474b26711a8b3e 1ee3cc88b09e8b17 45b6cc0f067624ec b232db750b01fe54 57fdea77b251b10f e95d3eeedb083bdf 109c41dba26cc965 4f787bf95735ff07 070b175cea8b6230 2e6087b91a041547 4605691099f1a9e2 b626c4b3bb7aeb8e ad9922bc3617cb42 7c669b88be5f98ae a7edb8b0063bec80 af4c081f89778d7c 7242ddae88e8d3af f1f80e575e1aab4a 5d115bc27636fd14 d19bc59433f69763 5ecd870d17e7f5b0 04dee4001cddc34a b6e377eeb3fb08e9 476970765105d93e 4558fe3d4fc6fe05 3aab9c6cf032f111 6e70c2d65f7c8cde b6ad63ac4291f93d 467ebbb29ead265c 05ac684d20a6bef0 9b71830f717e08bc b4f9d3773bec928f 66eeb64dc451e958 e357ebbfef5a342d f28707ac4b8e3e8c 854e8d691cb92e87 c0d57558e44cd754 424865c229c9e1ab b28e003b6819400b"

if __name__ == "__main__":

#    bigwords = cleartext.split(" ")
    bigwords = ciphertext.split(" ")
    i = 0
    for word in bigwords:
        first = word[0 : 8]
        second = word[8:]
        print("write_word(ADDR_R_DATA0 + %d, 32'h%s);" % (i, first))
        print("write_word(ADDR_R_DATA0 + %d, 32'h%s);" % (i + 1, second))
        i = i + 2
