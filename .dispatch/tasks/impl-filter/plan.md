# Implement Filter module — delta/polarity + new hierarchy layer

- [x] Read src/IO.h, src/IO.cpp, src/BP.h, src/BP.cpp for current structure
- [x] Create src/Filter.h/.cpp — owns one BP; applies delta/polarity (0=off, 1=Δ: in−processed, 2=Ø: −processed); also added to CMakeLists.txt target_sources
- [x] Update src/IO.h/.cpp — replace bpL_/bpR_ with filterL_/filterR_; pass outDeltaA/B param to each Filter per block
- [x] Write summary to .dispatch/tasks/impl-filter/output.md
