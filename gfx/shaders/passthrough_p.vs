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

mov  (8) g126<1>f r0<1>f {NoMask};         // Copy header to output (don't use execution mask)  
mov  (1) g126.5 0x0000FF00ud  {NoMask};    // Set the channel enable mask in header
mov  (8) g127<4>.xyzw g1<4>.xyzw;          // Pass through the vertices (unmodified)
send (8) null g126 0x26 0x04084000;        // Send EOT | URB , URB_WRITE_HWORD | URB_INTERLEAVED | HEADER_PRESENT

