# Back to College
---

## Input
1. Number of companies
2. Number of vaccination zones
3. Number of students
4. Probability of success for each company

## Explanation

### Company

- Each company takes `w` seconds (2-5) to prepare `r` batches (1-5) of `p` vaccines (10-20).
- After preparing vaccines, the company enters the delivery phase and resumes production only after usage of all produced vaccines.
- In the delivery phase company delivers the vaccines to vaccination zones and waits till the use of all produced
vaccines. After which, the company resumes production.

#### Functions

- `company`
- `delivery`

### Zone

- All zones are initially empty. They do not have students and vaccines. It continuously iterates to all the companies to acquire a batch of vaccines. After which, slots creation takes place by invoking create_slots.
- The number of slots created is a random value in 1 and min (8 , `r` ,`w`). Where `r` is remaining vaccines in the zone and `w` is the number of students waiting for a slot. After filling slots (in `wait_for_slots`) vaccination of students takes place. Creation of slots and vaccination repeats until the vaccines are empty.
- Vaccination zone goes to acquire vaccines and repeats the whole process when vaccines are empty.

#### Functions

- `Zone`
- `acquire_vaccines`
- `create_slots`
- `vaccinate_students`

### Student

- A student will be available for vaccination after a random interval of time. After arriving he searches for a slot by iterating all the vaccination zones. After acquiring a slot at a zone, student waits till he gets vaccinated (in
`vaccinate_students`) and goes to antibody checkup.
- In antibody checkup, if the student gets positive, he/she exits. Else if he/she tests negative, he/she leaves if it is his/her 3rd round else goes again for vaccination.

#### Functions

- `student`
- `wait_for_slot`
- `antibody_checkup`
