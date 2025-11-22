# Queen of Clocks

Requires [Egg v2](https://github.com/aksommerville/egg2) to build.

Code For A Cause Micro Jam, November 2025, theme "TIME MANIPULATION".

 - Single-screen platformer starring Dot Vine.
 - Lots of moving platforms and such.
 - B to use wand. Focusses any live object with a cardinal line of sight, and you can adjust its dt: Stop, Slow, Normal, Fast, Too Fast.
 
## Agenda

R 2025-11-20
F 2025-11-21
S 2025-11-22 Houseguest and concert. Won't be working much today.
U 2025-11-23 Aim to be finished by EOD. Try to reserve Monday for polish and stretch goals.
M 2025-11-24 Submit by EOD.
T 2025-11-25 Jam ends at 11:00.

## TODO

- [x] I think the current graphics are too fussy. Don't use an outline. Try a black sky and light walls (unoutlined sprites would read great against black).
- [x] Hero motion.
- [x] Physics. Generalize. We'll need a strong concept of seating, there's going to be lots of moving platforms.
- [x] Static hazards, death.
- [x] Allow to change wand direction if no pumpkin captured yet.
- [ ] One-way platforms? I didn't account for these in physics. Do we want?
- [ ] Wall hugging?
- [ ] Wall jump?
- [x] Moving platforms.
- - [x] How to communicate with the hero? It's important that riding platforms operate smoothly.
- [ ] Pushable blocks. (are we doing that?)
- [ ] Timed flamethrowers.
- [ ] Other moving hazards. Chainsaw on wheels, etc.
- [ ] Conveyor belts.
- [x] Soulballs.
- [ ] Password-entry puzzle where there are changing digits you can adjust.
- [ ] Real maps. 10..15 should be good.
- [ ] Don't wand thru walls. Or do. Or a fixed distance limit. Does it matter?
- [x] Wand.
- [x] Goal and advancement.
- [x] Music. I think just one song.
- [ ] Sound effects.
- [ ] Scorekeeping.
- [ ] End of game. Don't just loop to the start again!
