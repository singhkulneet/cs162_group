# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(file-ops-e2e) begin
(file-ops-e2e) create "test.txt"
(file-ops-e2e) open "test.txt"
(file-ops-e2e) wrote 239 bytes to "test.txt"
(file-ops-e2e) Check filesize = 239 "test.txt"
(file-ops-e2e) Tell is at 239 "test.txt"
(file-ops-e2e) Tell is at 0 after seek 0 "test.txt"
(file-ops-e2e) verified contents of "test.txt"
(file-ops-e2e) Check filesize is -1 after close "test.txt"
(file-ops-e2e) remove "test.txt"
(file-ops-e2e) open "test.txt" failed after remove
(file-ops-e2e) end
file-ops-e2e: exit(0)
EOF
pass;
