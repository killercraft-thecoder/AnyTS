/**
 * Defines Builtin Functions , for intiesnse and stuff only , not for AnyTS.
 */

// A AnyTS Value (Any)
type AnyTSValue = any;
// a Half Precision Number (fp16)
type HALF = number;

declare function isNaN(value:AnyTSValue);
/** Convert a Number to a half (double -> half) */
declare function half(value:AnyTSValue):HALF
/** Return the Size in Bytes of a Value */
declare function sizeof(value:AnyTSValue):number;