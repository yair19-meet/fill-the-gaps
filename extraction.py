import urllib.request
import wordfreq

print("1. Downloading 370,000 raw English words...")
url = "https://raw.githubusercontent.com/dwyl/english-words/master/words_alpha.txt"
response = urllib.request.urlopen(url)
raw_words = response.read().decode('utf-8').splitlines()

print("2. Filtering for Wild Card rules...")
# For a "rich" feel, we want words between 5 and 12 letters. 
# 4-letter words are often too simple, and 13+ is too long for a 2-minute timer!
valid_words = [w.lower() for w in raw_words if 5 <= len(w) <= 12 and w.isalpha()]

print("3. Mining the 'Rich Zone' (Zipf 3.0 to 5.5)...")
rich_dictionary = []

for word in valid_words:
    score = wordfreq.zipf_frequency(word, 'en')
    
    # This is the magic filter. 
    # It cuts off the top (too easy) and the bottom (too hard).
    if 3.0 <= score <= 5.5:
        rich_dictionary.append(word)

# Sort alphabetically for the C++ Trie
rich_dictionary.sort()

# Save to your data folder
with open('./data/dictionary_rich.txt', 'w', encoding='utf-8') as f:
    for word in rich_dictionary:
        f.write(word + '\n')

print(f"Success! Extracted {len(rich_dictionary)} incredibly rich game words.")