file(REMOVE_RECURSE
  "wakaama/examples/shared/tinydtls/aes/rijndael.c"
  "wakaama/examples/shared/tinydtls/ccm.c"
  "wakaama/examples/shared/tinydtls/crypto.c"
  "wakaama/examples/shared/tinydtls/dtls.c"
  "wakaama/examples/shared/tinydtls/dtls.h"
  "wakaama/examples/shared/tinydtls/dtls_debug.c"
  "wakaama/examples/shared/tinydtls/dtls_time.c"
  "wakaama/examples/shared/tinydtls/ecc/ecc.c"
  "wakaama/examples/shared/tinydtls/hmac.c"
  "wakaama/examples/shared/tinydtls/netq.c"
  "wakaama/examples/shared/tinydtls/peer.c"
  "wakaama/examples/shared/tinydtls/session.c"
  "wakaama/examples/shared/tinydtls/sha2/sha2.c"
  "wakaama/examples/shared/tinydtls/tinydtls.h"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/submodule_update.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
