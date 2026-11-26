# LIBERTY EXPANSION MANAGER MPC — v0.9.9

MPC is the commercial target. Mac Standalone remains a verification tool. Factory 1 has 100 unchanged sounds, four fixed expansion slots hold up to 100 each.

On MPC choose the new LIBRARIES tab. Select Collection 0–4, Categorie (Toutes or one of 12 categories), then Preset 1–100. Changing collection/category selects the first eligible row. Empty or out-of-range rows keep the current sound; audition is disabled unless the row matches the loaded Sound. Pre-ecoute uses a fixed MIDI note, 0.85 s, internally; it is not an external audio file. Existing Sound parameter and project state remain authoritative. Q-Links 1–5 map to the four controls and Sound. Effects settings are not replaced by browsing.

Cover illustrations are static slot art. A pack's Cover.png is stored separately; the pinned SDK does not provide a proven runtime image loading or file picker API. Native skin replacement strings are static, so runtime formatValue names/counts must be verified on actual MPC. No fictitious installation button: use the existing offline installer with the host stopped, then reopen the plugin. The demo pack uses three original synthetic sample renders, not new acoustic recordings. Copy its files into Content/Expansions/01 (or another unused slot). Never overwrite an occupied slot or move collections referenced in projects. Preserve Factory.pcm.

Commercial authorization still requires a provisioned production public key, verification tests and a release policy disabling development demo receipts; see licensing documentation. No commercial license has been issued.
