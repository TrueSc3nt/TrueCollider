# TrueCollider smoke / hit tests
Date: 2026-07-13T19:55:53.6073857+01:00

----- CPU address hit 1-2 -----
CMD: .\keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:100 -l compress -t 1 -x sequential -q
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode address
[+] Search compress only
[+] Thread : 1
[+] Search mode: sequential
[+] Quiet thread output
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] Setting search for btc adddress
[+] N = 4294967296
[+] Range 
[+] -- from : 0x1
[+] -- to   : 0x100
[+] Allocating memory for 2 elements: 0.00 MB
[+] Binary fuse filter for 2 elements.
[+] Collecting keys: 0.08 MB temp memory
[+] Building binary fuse filter from 2 keys... 
Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc

PASS  CPU address hit 1-2
----- CPU sol hit seed1 -----
CMD: .\keyhunt.exe -m address -c sol -f tests\sol_sample.txt -r 1:100 -t 1 -x sequential -q
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode address
[+] Setting search for Solana addresses.
[+] Thread : 1
[+] Search mode: sequential
[+] Quiet thread output
[+] Solana mode: ed25519 seed -> base58 pubkey address
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] N = 4294967296
[+] Range 
[+] -- from : 0x1
[+] -- to   : 0x100
[+] Binary fuse filter for 1 elements.
[+] Collecting keys: 0.08 MB temp memory
[+] Loaded 1 Solana target pubkey(s)
[+] Building Solana binary fuse filter from 1 keys... 
[!] Binary fuse failed, falling back to bloom filter

Hit! Solana seed: 1
pubkey: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt

End

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Mode: address (solana)
Private Key (seed hex): 1
Pub
...(truncated)
PASS  CPU sol hit seed1
----- CPU xpoint G -----
CMD: .\keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -r 1:100 -t 1 -x sequential -q
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode xpoint
[+] Thread : 1
[+] Search mode: sequential
[+] Quiet thread output
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] N = 4294967296
[+] Range 
[+] -- from : 0x1
[+] -- to   : 0x100
[+] Allocating memory for 1 elements: 0.00 MB
[+] Binary fuse filter for 1 elements.
[+] Collecting keys: 0.08 MB temp memory
[+] Building binary fuse filter from 1 keys... 
Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241a
...(truncated)
PASS  CPU xpoint G
----- CPU vanity hit 1Bg -----
CMD: .\keyhunt.exe -m vanity -v 1Bg -r 1:100 -l compress -t 1 -x sequential -q
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode vanity
[+] Added Vanity search : 1Bg
[+] Search compress only
[+] Thread : 1
[+] Search mode: sequential
[+] Quiet thread output
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] N = 4294967296
[+] Range 
[+] -- from : 0x1
[+] -- to   : 0x100
[+] Binary fuse filter for 2 elements.
[+] Collecting keys: 0.08 MB temp memory

Vanity Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6

Vanity Private Key: 90
pubkey: 028e3d1248c7657211d20291ce1798f490743f1bc852858e32d7efe2315fbc7671
Address 1BgXK1VnkLPGyTyfqCtvDpC8WVf9LsK5us
rmd160 752ac84300ff8ead7f957b8a3d6284dfe16ef158

Vanity Private Key: 12e
pubkey: 03654f313a31153e076e4e3f391d9fddcd9d3bce6705a8a806cfaaeb03678dfdc7
Address 1BgJygdYy8MpQeCzrfyiSaHL2Q1yGFpDAF
rmd160 75207caea9695462b56ca32037e473acc6206714

Vanity Private Key: fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0363e49
pubkey: 032134d4a76a5e0359d9508fc18fed915cd30eeaf14bf8feef60617a55bb1f0a5d
Address 1Bg54XMajAqFU1vteM8NMT8nvZUJ1coptm
rmd160 7514debe25ea828b1a2cf225c6c3d69e08d8aff5

Vanity Private Key: fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0363dd6
pubkey: 03f4729787abeac0f1249b1d3ac19bb0255764a854727b415f73e24c88a8a981c8
Address 1BgcopyY9F2RdEaNXboJ864waNVxjUrAdZ
rmd160 752f5eeebc50836d06e6d022a59e74a2ff2c0c4f

Vanity Private Key: fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0363ca6
pubkey: 02039a5a638244aef6432b1247d394d305ee594bd26ed2433a3a96c4ce5a158d0c
Address 1BgZGZXzqwW14JxE8VGybe56h4JNoofngu
rmd160 752c6aa3c2336eb5
...(truncated)
PASS  CPU vanity hit 1Bg
----- CPU bsgs dry -----
CMD: .\keyhunt.exe -m bsgs -f tests\125.txt -b 40 -n 1048576 -k auto -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] K factor: auto (from system RAM / -M)
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] Dry-run config summary:
    mode=bsgs crypto=0 threads=1 search=1
    GPU enabled=0 backend=none batch=8192
    memory=default(auto under CUDA)
    host_RAMâ‰ˆ30.0 GB â†’ BSGS recommend -n 0x100000000000 -k 1024 (will apply -k auto)
    current -k=1 (auto) -n=1048576
    file=tests\125.txt endomorphism=0
[+] Dry-run complete; exiting without search.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa
...(truncated)
PASS  CPU bsgs dry
----- CPU pubkey2addr dry -----
CMD: .\keyhunt.exe -m pubkey2addr -f tests\_btc_1to2.txt -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode pubkey2addr (random pubkey->address search)
[+] Defaulting to -x random
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] Dry-run config summary:
    mode=other crypto=0 threads=1 search=1
    GPU enabled=0 backend=none batch=8192
    memory=default(auto under CUDA)
    host_RAMâ‰ˆ30.0 GB â†’ BSGS recommend -n 0x100000000000 -k 1024
    file=tests\_btc_1to2.txt endomorphism=0
[+] Dry-run complete; exiting without search.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a
...(truncated)
PASS  CPU pubkey2addr dry
----- CPU minikeys dry -----
CMD: .\keyhunt.exe -m minikeys -f tests\_btc_1to2.txt -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode minikeys
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] Dry-run config summary:
    mode=other crypto=0 threads=1 search=1
    GPU enabled=0 backend=none batch=8192
    memory=default(auto under CUDA)
    host_RAMâ‰ˆ30.0 GB â†’ BSGS recommend -n 0x100000000000 -k 1024
    file=tests\_btc_1to2.txt endomorphism=0
[+] Dry-run complete; exiting without search.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Mode: address (solana)
Private Key (seed hex): 1
Public
...(truncated)
PASS  CPU minikeys dry
----- CPU mnemonic dry -----
CMD: .\keyhunt.exe -m mnemonic -f tests\_btc_1to2.txt -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode mnemonic (BIP39)
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] Dry-run config summary:
    mode=other crypto=0 threads=1 search=1
    GPU enabled=0 backend=none batch=8192
    memory=default(auto under CUDA)
    host_RAMâ‰ˆ30.0 GB â†’ BSGS recommend -n 0x100000000000 -k 1024
    file=tests\_btc_1to2.txt endomorphism=0
[+] Dry-run complete; exiting without search.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Mode: address (solana)
Private Key (seed hex): 1
...(truncated)
PASS  CPU mnemonic dry
----- CPU poetry dry -----
CMD: .\keyhunt.exe -m poetry -f tests\_btc_1to2.txt -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode poetry (hex encoding)
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] Dry-run config summary:
    mode=other crypto=0 threads=1 search=1
    GPU enabled=0 backend=none batch=8192
    memory=default(auto under CUDA)
    host_RAMâ‰ˆ30.0 GB â†’ BSGS recommend -n 0x100000000000 -k 1024
    file=tests\_btc_1to2.txt endomorphism=0
[+] Dry-run complete; exiting without search.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Mode: address (solana)
Private Key (seed he
...(truncated)
PASS  CPU poetry dry
----- CPU brainwallet dry -----
CMD: .\keyhunt.exe -m brainwallet -f tests\_btc_1to2.txt -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode brainwallet
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[+] Dry-run config summary:
    mode=other crypto=0 threads=1 search=1
    GPU enabled=0 backend=none batch=8192
    memory=default(auto under CUDA)
    host_RAMâ‰ˆ30.0 GB â†’ BSGS recommend -n 0x100000000000 -k 1024
    file=tests\_btc_1to2.txt endomorphism=0
[+] Dry-run complete; exiting without search.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Mode: address (solana)
Private Key (seed hex): 1
Pub
...(truncated)
PASS  CPU brainwallet dry
----- CUDA address dry -----
CMD: .\keyhunt_cuda.exe -m address -f tests\_btc_1to2.txt -U cuda -M auto -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode address
[+] GPU backend set to CUDA
[+] GPU/search memory: auto (from free VRAM)
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1
[+] D[E] tcuda_hash160_33_selftest: hash mismatch.
  got: 6fc6f0e46cf9fb4582176fecad6f20545dc2461a
  exp: 751e76e8199196d454941c45d1b3a323f1433bd6
[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.
[+] CUDA secp256k1 self-test passed (GPU EC ok; host filter mode).
[+] CUDA secp256k1 path ready (GPU EC + host hash/bloom).
[+] CUDA ed25519 (Solana) self-test passed (GPU SHA512 + host ge).
[+] GPU memory plan: budget 2128.8 MB (auto) | VRAM free 3548.0 / total 8191.4 MB | batch up to 1048576 keys | launch chunk 65536 | ~6.1 MB device buffers
[+] CUDA backend selected (1 device), effective batch 1048576 keys.
[+] GPU EC enabled for mode (batch 1048576 keys; -G / -M to tune).

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241
...(truncated)
PASS  CUDA address dry
----- CUDA address hit 1-2 -----
CMD: .\keyhunt_cuda.exe -m address -f tests\_btc_1to2.txt -r 1:100 -l compress -U cuda -G 256 -M 512 -t 1 -x sequential -q
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode address
[+] Search compress only
[+] GPU backend set to CUDA
[+] GPU batch size hint: 256 (clamped by -M / free VRAM)
[+] GPU/search memory budget: 512 MB
[+] Thread : 1
[+] Search mode: sequential
[+] Quiet thread output
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1
[+] Setting search for btc adddress
[+] N = 4294967296
[+] Range 
[+] -- from : 0x1
[+] -- to   : 0x100
[+] Allocating memory for 2 elements: 0.00 MB
[+] Binary fuse filter for 2 elements.
[+] Collecting keys: 0.08 MB temp memory
[+] Building binary fuse filter from 2 keys... 
[!] Binary fuse failed, falling back to bloom filter
[+] Sorting data ... done! 2 values were loaded and sorted

Hit! Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6

Hit! Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
[E] tcuda_hash160_33_selftest: hash mismatch.
  got: 6fc6f0e46cf9fb4582176fecad6f20545dc2461a
  exp: 751e76e8199196d454941c45d1b3a323f1433bd6
[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.
[+] CUDA secp256k1 self-test passed (GPU EC ok; host filter mode).
[+] CUDA secp256k1 path ready (GPU EC + host hash/bloom).
[+] CUDA ed25519 (Solana) self-test passed (GPU SHA512 + host ge).
[+] GPU memory plan: budget 512.0 MB (user) | VRAM free 3548.0 / total 8191.4 MB | batch up to 1048576 keys | launch chunk 65536 | ~6.1 MB device buffers
[+] CUDA ba
...(truncated)
PASS  CUDA address hit 1-2
----- CUDA vanity hit 1Bg -----
CMD: .\keyhunt_cuda.exe -m vanity -v 1Bg -r 1:100 -l compress -U cuda -G 256 -t 1 -x sequential -q
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode vanity
[+] Added Vanity search : 1Bg
[+] Search compress only
[+] GPU backend set to CUDA
[+] GPU batch size hint: 256 (clamped by -M / free VRAM)
[+] Thread : 1
[+] Search mode: sequential
[+] Quiet thread output
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1
[+] N = 4294967296
[+] Range 
[+] -- from : 0x1
[+] -- to   : 0x100
[+] Binary fuse filter for 2 elements.
[+] Collecting keys: 0.08 MB temp memory

Vanity Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6

Vanity Pri[E] tcuda_hash160_33_selftest: hash mismatch.
  got: 6fc6f0e46cf9fb4582176fecad6f20545dc2461a
  exp: 751e76e8199196d454941c45d1b3a323f1433bd6
[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.
[+] CUDA secp256k1 self-test passed (GPU EC ok; host filter mode).
[+] CUDA secp256k1 path ready (GPU EC + host hash/bloom).
[+] CUDA ed25519 (Solana) self-test passed (GPU SHA512 + host ge).
[+] GPU memory plan: budget 2128.8 MB (auto) | VRAM free 3548.0 / total 8191.4 MB | batch up to 1048576 keys | launch chunk 65536 | ~6.1 MB device buffers
[+] CUDA backend selected (1 device), effective batch 256 keys.
[+] GPU EC enabled for mode (batch 256 keys; -G / -M to tune).

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1
...(truncated)
PASS  CUDA vanity hit 1Bg
----- CUDA xpoint G -----
CMD: .\keyhunt_cuda.exe -m xpoint -f tests\_xpoint_g.txt -r 1:100 -U cuda -G 256 -t 1 -x sequential -q
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode xpoint
[+] GPU backend set to CUDA
[+] GPU batch size hint: 256 (clamped by -M / free VRAM)
[+] Thread : 1
[+] Search mode: sequential
[+] Quiet thread output
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1
[+] N = 4294967296
[+] Range 
[+] -- from : 0x1
[+] -- to   : 0x100
[+] Allocating memory for 1 elements: 0.00 MB
[+] Binary fuse filter for 1 elements.
[+] Collecting keys: 0.08 MB temp memory
[+] Building binary fuse filter from 1 keys... 
[!] Binary fuse failed, falling back to bloom filter
[+] Sorting data ... done! 1 values were loaded and sorted

Hit! Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5
[E] tcuda_hash160_33_selftest: hash mismatch.
  got: 6fc6f0e46cf9fb4582176fecad6f20545dc2461a
  exp: 751e76e8199196d454941c45d1b3a323f1433bd6
[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.
[+] CUDA secp256k1 self-test passed (GPU EC ok; host filter mode).
[+] CUDA secp256k1 path ready (GPU EC + host hash/bloom).
[+] CUDA ed25519 (Solana) self-test passed (GPU SHA512 + host ge).
[+] GPU memory plan: budget 2128.8 MB (auto) | VRAM free 3548.0 / total 8191.4 MB | batch up to 1048576 keys | launch chunk 65536 | ~6.1 MB device buffers
[+] CUDA backend selected (1 device), effective batch 256 keys.
[+] GPU EC enabled for mode (batch 256 keys; -G / -M to tune).
[+] Bloom ready for GPU-EC path (35944 bytes, 20 hashes, host filter).

Private Key: 1
pubkey: 0279be667ef9d
...(truncated)
PASS  CUDA xpoint G
----- CUDA pubkey2addr dry -----
CMD: .\keyhunt_cuda.exe -m pubkey2addr -f tests\_btc_1to2.txt -U cuda -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode pubkey2addr (random pubkey->address search)
[+] Defaulting to -x random
[+] GPU backend set to CUDA
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5
Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


P
...(truncated)
PASS  CUDA pubkey2addr dry
----- CUDA minikeys dry -----
CMD: .\keyhunt_cuda.exe -m minikeys -f tests\_btc_1to2.txt -U cuda -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode minikeys
[+] GPU backend set to CUDA
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1
[E] tcuda_hash160_33_selftest: hash mismatch.
  got: 6fc6f0e46cf9fb4582176fecad6f20545dc2461a
  exp: 751e76e8199196d454941c45d1b3a323f1433bd6
[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5
Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key 
...(truncated)
PASS  CUDA minikeys dry
----- CUDA mnemonic dry -----
CMD: .\keyhunt_cuda.exe -m mnemonic -f tests\_btc_1to2.txt -U cuda -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode mnemonic (BIP39)
[+] GPU backend set to CUDA
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1
[E] tcuda_hash160_33_selftest: hash mismatch.
  got: 6fc6f0e46cf9fb4582176fecad6f20545dc2461a
  exp: 751e76e8199196d454941c45d1b3a323f1433bd6
[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5
Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Priv
...(truncated)
PASS  CUDA mnemonic dry
----- CUDA poetry dry -----
CMD: .\keyhunt_cuda.exe -m poetry -f tests\_btc_1to2.txt -U cuda -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode poetry (hex encoding)
[+] GPU backend set to CUDA
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5
Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce
...(truncated)
PASS  CUDA poetry dry
----- CUDA brainwallet dry -----
CMD: .\keyhunt_cuda.exe -m brainwallet -f tests\_btc_1to2.txt -U cuda -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode brainwallet
[+] GPU backend set to CUDA
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1
[E] tcuda_hash160_33_selftest: hash mismatch.
  got: 6fc6f0e46cf9fb4582176fecad6f20545dc2461a
  exp: 751e76e8199196d454941c45d1b3a323f1433bd6
[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5
Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private K
...(truncated)
PASS  CUDA brainwallet dry
----- CUDA bsgs dry -----
CMD: .\keyhunt_cuda.exe -m bsgs -f tests\125.txt -b 40 -n 1048576 -k 1 -U cuda -M auto -y
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] K factor 1
[+] GPU backend set to CUDA
[+] GPU/search memory: auto (from free VRAM)
[+] Dry-run: will print resolved config and exit (no search)
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5
Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
rmd160 751e76e8199196d454941c45d1b3a323f1433bd6
Private Key: 2
pubkey: 02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5
Address 1cMh228HTCiwS8ZsaakH8A8wze1JR5ZsP
rmd160 06afd46bcdfd22ef94ac122aa11f241244a37ecc
Private Key: 1
pubkey: 0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
Address 1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
rmd160 91b24bf9f5288532960ac687abb035127b1d28a5

Mode: address (solana)
Private Key (seed hex): 1
Public Key: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt


Private Key: 1
pubkey
...(truncated)
PASS  CUDA bsgs dry
----- CUDA sol hit seed1 -----
CMD: .\keyhunt_cuda.exe -m address -c sol -f tests\sol_sample.txt -r 1:100 -U cuda -G 256 -M 512 -t 1 -x sequential -q
[+] Version TrueCollider Search Modes + Binary Fuse Filters, developed & modified by TrueScent
[+] Mode address
[+] Setting search for Solana addresses.
[+] GPU backend set to CUDA
[+] GPU batch size hint: 256 (clamped by -M / free VRAM)
[+] GPU/search memory budget: 512 MB
[+] Thread : 1
[+] Search mode: sequential
[+] Quiet thread output
[+] Solana mode: ed25519 seed -> base58 pubkey address
[+] CPU detect: SSSE3 â†’ selected SSE â†’ using SSE 4-wide hash160 (auto)
[CUDA] Devices detected: 1
[+] N = 4294967296
[+] Range 
[+] -- from : 0x1
[+] -- to   : 0x100
[+] Binary fuse filter for 1 elements.
[+] Collecting keys: 0.08 MB temp memory
[+] Loaded 1 Solana target pubkey(s)
[+] Building Solana binary fuse filter from 1 keys... 
[!] Binary fuse failed, falling back to bloom filter

Hit! Solana seed: 1
pubkey: 4cb5abf6ad79fbf5abbccafcc269d85cd2651ed4b885b5869f241aedf0a5ba29
Address: 6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt
[E] tcuda_hash160_33_selftest: hash mismatch.
  got: 6fc6f0e46cf9fb4582176fecad6f20545dc2461a
  exp: 751e76e8199196d454941c45d1b3a323f1433bd6
[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.
[+] CUDA secp256k1 self-test passed (GPU EC ok; host filter mode).
[+] CUDA secp256k1 path ready (GPU EC + host hash/bloom).
[+] CUDA ed25519 (Solana) self-test passed (GPU SHA512 + host ge).
[+] GPU memory plan: budget 512.0 MB (user) | VRAM free 3548.0 / total 8191.4 MB | batch up to 1048576 keys | launch chunk 65536 | ~6.1 MB device buffers
[+] CUDA backend selected (1 device), effective batch 256 keys.
[+] GPU Solana path enabled (CUDA SHA512 + host ed25519 ge; batch 256).

Private Key: 1
pubkey: 0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
Address 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26
...(truncated)
PASS  CUDA sol hit seed1

## Summary
PASS=21 FAIL=0
