import wikipedia
import re
import pandas as pd
from wordfreq import zipf_frequency

# 1. Pick your exact game topics!
topics = [
    "Geopolitics", "International relations", "Politics", "Imperialism", 
    "Globalization", "Diplomacy", "Nuclear Weapons", "United Nations", 
    "NATO", "European Union", "Cold War", "Nationalism", "Liberalism", "Economics", "Math", "Algorithms",
    "Games", "Food", "AI", "Physics", "Cryptography", "Computer Science", "Machine Learning", "Programming", 
    "Philosophy", "Sports", "Leaders", "Technology", "History", "Biology", "Chemistry", "Mathematics", "Literature", "Art", "Music", "Theatre", "Dance", "Film", "Television", "Radio", "Journalism", "Publishing", "Academia", "Education", "Training", "Learning", "Teaching", "Instruction", "Education system", "Curriculum", "Assessment", "Evaluation", "Grading", "Scoring", "Measurement", "Quantification", 
    "Statistics", "Probability", "Calculus", "Geometry", "Algebra", "Trigonometry", "Statistics", "Calculus", "Geometry", "Algebra", "Trigonometry", "Animals", "Plants", "Veganism", 
    "Fruit"
]
raw_text = ""

print("Downloading articles from Wikipedia...")
for topic in topics:
    try:
        # auto_suggest=False stops Wikipedia from hallucinating typos
        page = wikipedia.page(topic, auto_suggest=False)
        raw_text += " " + page.content
        print(f"Downloaded: {topic}")
    except wikipedia.exceptions.PageError:
        print(f"Skipping {topic}: Page does not exist.")
    except wikipedia.exceptions.DisambiguationError as e:
        print(f"Skipping {topic}: Too broad.")

print("\nExtracting and cleaning words...")
# Extract all 4+ letter words
extracted_words = pd.Series(re.findall(r'\b([a-z]{4,})\b', raw_text.lower()))

# Count raw frequencies first
word_counts = extracted_words.value_counts()

# 2. THE ZIPF FILTER (The Magic Bouncer)
# We only want words that appear at least 2 times in our Wikipedia text
# BUT we want to reject words that are too common in everyday English (Zipf > 4.5)
final_words = []

for word, count in word_counts.items():
    if count >= 2: # Has to actually be relevant to the Wikipedia articles
        zipf_score = zipf_frequency(word, 'en')
          
        # > 0 ensures it's a real word. < 4.5 strips out "structure", "success", etc.
        if 0 < zipf_score < 5: 
            final_words.append(word)
 
final_words.sort()

# 3. Save to C++ Backend
print(f"\nSuccess! Saved {len(final_words)} words.")
with open('./data/full_vocab.txt', 'w', encoding='utf-8') as file:
    for word in final_words:
        file.write(word + '\n')