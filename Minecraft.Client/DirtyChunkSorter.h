#pragma once
class Chunk;
class Mob;

class DirtyChunkSorter
{
private:
	shared_ptr<Mob> cameraEntity;
	int playerIndex; // 4J added

public:
    DirtyChunkSorter(shared_ptr<Mob> cameraEntity, int playerIndex);	// 4J - added player index
	bool operator()(const Chunk *a, const Chunk *b) const;
};