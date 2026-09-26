#ifndef FIGHTER_ENTITIES_H
#define FIGHTER_ENTITIES_H

#include "fighter_types.h"

// Fighter
void InitFighter(Fighter *f, CharacterType charType, float x, bool facingRight);
void UpdateFighter(Fighter *f, float dt);
void DrawFighter(Fighter *f, float shakeX, float shakeY);
void FighterSetState(Fighter *f, FighterState state);
void FighterTakeDamage(Fighter *f, int damage, float knockback, bool fromRight);
Rectangle GetFighterHurtbox(Fighter *f);
Hitbox GetFighterAttackHitbox(Fighter *f);

// Projectile
void FighterSpawnProjectile(FighterGame *game, int owner, Vector2 pos, Vector2 vel, int damage, Color color);
void FighterUpdateProjectiles(FighterGame *game, float dt);
void FighterDrawProjectiles(FighterGame *game);

// Particle
void FighterSpawnParticle(FighterGame *game, Vector2 pos, Vector2 vel, Color color, float size, float lifetime, int type);
void FighterSpawnHitSparks(FighterGame *game, Vector2 pos, int count);
void FighterSpawnDust(FighterGame *game, Vector2 pos, int count);
void FighterSpawnBlockSparks(FighterGame *game, Vector2 pos, int count);
void FighterUpdateParticles(FighterGame *game, float dt);
void FighterDrawParticles(FighterGame *game);

#endif // FIGHTER_ENTITIES_H
