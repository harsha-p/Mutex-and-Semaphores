# Musical Mayhem
---

## Input

- Number of performers
- Number of acoustic, electric stages
- Number of coordinators
- Minimum, maximum performance time
- Wait time for each performer
- Name, instrument type, the arrival time for each performer

## Explanation

- `getsastage` is the primary function. The instrument decides the type of performer. Type is `0` if the performer performs only on the acoustic stage or `1` if the performer performs only on the electric stage else it is `2`.
- Based on the type of performer corresponding timer threads (acoustic/electric) are created.
- Also, for a singer, timer thread is created which searches for a musician performing a solo (waits for formusician semaphore) and posts for singer semaphores as a signal. forsinger semaphore is taken back if a musician is not available.
- Performers leave if they do not get corresponding stage or musician (for a singer) in time.
- Initially, stage of a performer is –1. When timer threads are created, the thread which first received the semaphore sets the value of stage to 0 for acoustic, 1 for electric and 2 for musician.
- The timer threads for a stage also invokes performance function for the performer after getting a stage.
- If a performer is performing, he/she creates a timer thread to search for a singer who is not
performing. The performing musician sends a signal by posting formusician semaphore and waits for forsinger semaphore. If found, a singer performance time is increased by 2 seconds else formusician semaphore is taken back.
- After completion of performance, a thread is created to collect t-shirt from coordinators (for both singers and musicians who performed).

## Functions

- `getastage` – initialize performer, creates timer threads.
- `timerfor_astage` – timer for acoustic stage.
- `timerfor_estage` – timer for electric stage.
- `timerfor_musician` – timer to search for musician performing solo.
- `Perform` – start performing on a stage, create a thread to find a singer
- `checksinger` – timer for finding a singer.
- `taketshirt` – take a t-shirt from a coordinator.
