#!/usr/bin/env python
#coding:utf-8
# File:    lang_detect_nltk.py
# Author:  Dr. Ivan S. Zapreev
# Purpose: Detecting language using a stopwords based approach
#          This script requires one parameter:
#             ${1} - is the input file name with a UTF-8 text
#          This script tries to detect the language of the text 
#          based on the first 1024 UTF-8 characters thereof. The
#          detected text language name (ASCII) is printed to the
#          standard output. If the language is not detected then
#          the script exits with an error code.
# Derived: From
#          http://blog.alejandronolla.com/2013/05/15/detecting-text-language-with-python-and-nltk/
#
# Visit my Linked-in profile:
#      <https://nl.linkedin.com/in/zapreevis>
# Visit my GitHub:
#      <https://github.com/ivan-zapreev>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Created on November 14, 2016, 11:07 AM
#

import codecs
from sys import exit
from sys import argv
from itertools import islice

#Make sure we only work with the UTF-8 strings
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

try:
    from nltk.tokenize import word_tokenize
    from nltk.corpus import stopwords
except ImportError:
    print '[!] You need to install nltk (http://nltk.org/index.html)'
    exit(1)

#----------------------------------------------------------------------
def _calculate_languages_ratios(text):
    """\
    Calculate probability of given text to be written in several languages and
    return a dictionary that looks like {'french': 2, 'spanish': 4, 'english': 0}
    
    @param text: Text whose language want to be detected
    @type text: str
    
    @return: Dictionary with languages and unique stopwords seen in analyzed text
    @rtype: dict
    """

    languages_ratios = {}

    #Tokenize into words
    tokens = word_tokenize(text)
    #Lowercase the words
    words = [word.lower() for word in tokens]

    # Compute per language included in nltk number of unique stopwords appearing in analyzed text
    for language in stopwords.fileids():
        stopwords_set = set(stopwords.words(language))
        words_set = set(words)
        common_elements = words_set.intersection(stopwords_set)

        languages_ratios[language] = len(common_elements) # language "score"

    return languages_ratios


#----------------------------------------------------------------------
def detect_language(text):
    """\
    Calculate probability of given text to be written in several languages and
    return the highest scored. In case the language can not be detected the
    script exits with an error code.
    
    It uses a stopwords based approach, counting how many unique stopwords
    are seen in analyzed text.
    
    @param text: Text whose language want to be detected
    @type text: str
    
    @return: Most scored language guessed
    @rtype: str
    """

    ratios = _calculate_languages_ratios(text)

    most_rated_language = max(ratios, key=ratios.get)
    
    #Check if the language was indeed dected
    if ratios[most_rated_language] == 0:
        print 'Could not detect the source language'
        exit(1)

    return most_rated_language


if __name__=='__main__':

    """
    Only read a small buffer of 1024 UTF-8 characters to do
    language detection, no need to do it on an entire file.
    """
    text = ''
    with codecs.open(argv[1], "r", "utf-8") as file:
        text = file.read(1024)
    
    language = detect_language(text)

    #Return the detected language capitalizing the first letter
    print language[0].capitalize() + language[1:]
