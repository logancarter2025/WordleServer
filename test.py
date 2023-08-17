'''
LOGIC FOR GIVING USER FEEDBACK IN PYTHON
TO REPLICATE IN C, NEED TO IMPLIMENT MAP + MAP FUNCTIONS
'''

secret_word = 'udder'
my_guess = input("Enter a five letter word: ").strip().lower()

s = dict()
for i in range(len(secret_word)):
    if secret_word[i] not in s:
        s[secret_word[i]] = [i]
        continue
    s[secret_word[i]].append(i)

g = dict()
times_guessed = dict()
for i in range(len(my_guess)):
    if my_guess[i] not in g:
        g[my_guess[i]] = [i]
        times_guessed[my_guess[i]] = 0
        continue
    g[my_guess[i]].append(i)


hints = ['-', '-', '-', '-', '-']

#Now check for letters that are in correct spot
for i in range(0, len(my_guess)):
    if my_guess[i] in s and i in s[my_guess[i]]:
        hints[i] = my_guess[i].upper()
        times_guessed[my_guess[i]] += 1

#Now check for letters that are in word but in wrong spot
for i in range(0, len(my_guess)):
    if my_guess[i] in s and i not in s[my_guess[i]] and times_guessed[my_guess[i]] < len(s[my_guess[i]]):
            hints[i] = my_guess[i]
            times_guessed[my_guess[i]] += 1


print(''.join(hints))
