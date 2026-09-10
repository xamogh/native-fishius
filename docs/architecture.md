# Architecture

## Authoritative domain

`domain.hpp` and `domain.cpp` contain content records, state, lifecycle calculations, movement, food consumption, transactions, and local progression. The domain receives simulation durations and calendar values. It does not open files, read an operating-system clock, create an SDL object, or decide how to draw a screen.

The Session owns one Domain. Presentation reads state and submits Commands. A command returns a structured Result. The command wrapper restores its saved pre-command state on an expected failure or an exception. Receipts and sounds are consequences of successful commands, never the cause of wallet mutations.

Fish IDs survive stashing, restoration, saving, and revival. Container indices and renderer pointers are not persistent identity. Currency arithmetic is checked and bounded. The explicit revival overflow exception is separate from ordinary capacity gates.

## Time

Lifecycle advancement operates on exact millisecond deadlines. Hatching, starvation pause, sickness, and death are not rounded to movement ticks. Movement uses a fixed 20-millisecond step. Long frame recovery bounds movement work without intentionally dropping care time. Cosmetic mesh waves do not choose which fish eats food.

Only the active tank has visible movement and loose food. Care advances in all owned tanks. Stashed fish retain paused deadlines. Loose food is transient and is cleared on tank changes and suspension.

## Persistence and session

Storage encodes explicit JSON fields and validates a temporary candidate before installation. Session coordinates elapsed time, suspension, offline catch-up, commands, and save checkpoints. It is the boundary between the pure domain and operating-system time or storage.

## Presentation

View owns the panel and tool state, gesture ownership, pressed states, tool animations, receipts, and toasts. It keeps entity IDs instead of references across mutations. Canvas owns SDL resources, font and texture caches, drawing operations, native text, and the deforming fish mesh.

Rendering never computes a sale reward. Scene pixels never become the authoritative location of an inventory item or the authoritative wallet balance.

The implementation uses a small number of modules rather than a general-purpose engine or an entity-component framework. Some classes and functions should receive a further maintainability review before production use; source presence alone is not an AAA-quality certification.
