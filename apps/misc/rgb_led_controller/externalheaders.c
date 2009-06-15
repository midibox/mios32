// should be changed for your own application
// Headers used by MIDIbox applications are documented here:
// http://svnmios.midibox.org/filedetails.php?repname=svn.mios&path=%2Ftrunk%2Fdoc%2FSYSEX_HEADERS
// if you decide to use "F0 00 00 7E" prefix, please ensure that your
// own ID (here: 0x7f) will be entered into this document.
// Otherwise please use a different header

const unsigned char ab_sysex_header[2] = { 0xf0, 0x0A };
const unsigned char ab_sysex_footer = 0xf7;