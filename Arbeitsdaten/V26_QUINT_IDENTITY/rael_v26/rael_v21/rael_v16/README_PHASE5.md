# Rael – Phase 5 (Immutable CoreRing + Signed Pack Installer, C++ only)
Adds:
- CoreRing: IchBin + Ethik + MathCore anchors compiled into binary and verified on boot.
- Pack format: .rael (key=value lines). No JSON.
- Pack signature: SHA256( key || payload_material ). Key lives inside CoreRing.
CLI:
- pack verify <file>
- pack install <file>  (copies to ./packs and optional payload dir)
Notes:
- Hot-swap of protected core names is blocked: IchBinKern, EthikKern, MathCore.
