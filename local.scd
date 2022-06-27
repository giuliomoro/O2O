~displayOSC = NetAddr.new("bela.local", 7562);
// /targetMode can be:
// * 0: single-display mode
// * 1: each mode, the first argument to each message is the target display
// * 2: stateful mode. you can send `/target` messages to change the target display for the following messages
~displayOSC.sendMsg('/targetMode', 0); // single display
~displayOSC.sendMsg('/targetMode', 2); // stateful mode: set target display with: /target
~displayOSC.sendMsg('/target', 2); // only works in stateful mode

~displayOSC.sendMsg('/osc-test');
~displayOSC.sendMsg('/number', 12);
~displayOSC.sendMsg('/display-text', 'one', 'two', 'three');
~displayOSC.sendMsg('/parameters', 0.1, 1.0, 1.0);
~displayOSC.sendMsg('/lfos', 0.1, 0.4, 1.0);
~displayOSC.sendMsg('/waveform', 0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, 0.0);

// per-message target ("each")
~displayOSC.sendMsg('/targetMode', 1);
// in "each" mode, the first argument to each message is the destination display
~tar = 2;
~displayOSC.sendMsg('/osc-test', ~tar);
~displayOSC.sendMsg('/number', ~tar, 12);
~displayOSC.sendMsg('/display-text', ~tar, 'one', 'two', 'three');
~displayOSC.sendMsg('/parameters', ~tar, 0.1, 1.0, 1.0);
~displayOSC.sendMsg('/lfos', ~tar, 0.1, 0.4, 1.0);
~displayOSC.sendMsg('/waveform', ~tar, 0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, 0.0);
