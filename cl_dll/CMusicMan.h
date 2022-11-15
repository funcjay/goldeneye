#pragma once

//
//	SoLoud music manager, written by Jay - 2022
//	You are free to use and modify this code as long as credit is given.
//

void MusicMan_Play(const char* file, bool loop, bool fade);
void MusicMan_Paused(bool pause);

void MusicMan_Start();
void MusicMan_Update();
void MusicMan_Shutdown();
