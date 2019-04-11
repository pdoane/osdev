// Position Passthrough Vertex Shader
//
// Input
//
//  7    6    5    4    3    2    1    0       GRF
//               VS Header                   - r0
// v1.w v1.z v1.y v1.x v0.w v0.z v0.y v0.z   - r1
//
// Output
//  7    6    5    4    3    2    1    0
//              URB Header                   - r126
// v1.w v1.z v1.y v1.x v0.w v0.z v0.y v0.z   - r127
//

// Mode: SIMD4x2

mov  (8) g125<1>f r0<1>f {NoMask};         // Copy header to output (don't use execution mask)
mov  (1) g125.20UD 0x0000FF00ud {NoMask};  // Set the channel enable mask in header
mov  (8) g126<1>UD 0x00000000UD;           // D0-D3 of VUE Vertex Header for URB handle 0 and 1
mov  (8) g127<1>.xyzw g1<4>.xyzw {align16};// Pass through the vertices (unmodified)
send (8) null g125 0x26 0x06084000;        // Send EOT | URB , URB_WRITE_HWORD | URB_INTERLEAVED | HEADER_PRESENT

