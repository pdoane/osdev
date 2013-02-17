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

mov  (8) r126<8>:f r0<8>:f {NoMask}       // Copy header to output (don't use execution mask)  
mov  (1) r126.5 0x0000FF00:ud  {NoMask}   // Set the channel enable mask in header
mov  (8) r127<4>.xyzw:f r1<4>.xyzw:f      // Pass through the vertices (unmodified)
send (8) null r126:f 0x26 0x04084000      // Send EOT | URB , URB_WRITE_HWORD | URB_INTERLEAVED | HEADER_PRESENT

