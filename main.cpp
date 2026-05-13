/*
insert 2M nodes
custom allocator or posix_memalign to not allow OS to put it to random places (new does not guarantee alignment)
shuffle search queries to not allow prefetching
*/