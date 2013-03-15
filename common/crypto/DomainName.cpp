// #include <stdlib.h>
// #include <assert.h>
// #include <string.h>

// #ifndef _WIN32
// #include <math.h>  // For min()
// #endif

#include "LiteLib.h"
#include "DomainName.h"
#include "CryptoException.h"

#ifndef _WIN32
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#endif // _WIN32

/* 
 * Expects a DNS-style byte array consisting of one or more labels.
 * Each label begins with a 1-byte count of the number of 
 * bytes that follows.  Name should be terminated by a byte of 0
 * (a label with length 0), which is the root label
 * Assumes the memory range [name, name+size) is valid
 */
DomainName::DomainName(ByteStream &dnsbuffer) {
	this->len = 0;	
	this->num_labels = 0;

	bool wellFormed = false;
	while (this->len < MAX_DNS_NAME_LEN) {
		uint8_t labelLen = dnsbuffer[this->len];		

		// Copy the label's bytes (offset by 1 to skip past the labelLen)
		for (uint8_t i = 0; i < labelLen; i++) {			
			this->nameBytes[this->len + i] = dnsbuffer[this->len + i + 1];
		}

		if (labelLen == 0) {	// It's the root label that marks the end of the name
			if (this->len == 0) {
				// The entire name is the root label, which we represent as " "
				this->nameBytes[this->len] = ' ';
				this->nameBytes[this->len + 1] = '\0';
			} else {
				// We've been adding a series of labels, each followed by '.'
				// Remove the final period
				this->nameBytes[this->len - 1] = '\0';
			} 

			this->len++;
			wellFormed = true;			
			break;
		} else if (! (labelLen == 1 && dnsbuffer[this->len + 1] == '*')) {
			// It's not the root label and it's not a wildcard label, so count it
			this->num_labels++;
		}
		this->nameBytes[this->len + labelLen] = '.';
		this->len += labelLen + 1;
	}

	if(! (this->len <= MAX_DNS_NAME_LEN && wellFormed)) {
		Throw(CryptoException::BAD_INPUT, "DomainName badly formatted or too long");
	}
}


/* 
 * Expects a DNS-style byte array consisting of one or more labels.
 * Each label begins with a 1-byte count of the number of 
 * bytes that follows.  Name should be terminated by a byte of 0
 * (a label with length 0), which is the root label
 * Assumes the memory range [name, name+size) is valid
 */
DomainName::DomainName(const uint8_t* dnsbuffer, uint32_t size) {
	this->len = 0;	
	this->num_labels = 0;
	if (dnsbuffer == NULL || size == 0) {
		Throw(CryptoException::BAD_INPUT, "DomainName too small");
	}

	bool wellFormed = false;

	while (this->len < min(MAX_DNS_NAME_LEN, size)) {
		uint8_t labelLen = dnsbuffer[this->len];

		if ((uint32_t)(this->len + labelLen + 1) > size) { Throw(CryptoException::BAD_INPUT, "DomainName exceeded bounds"); }

		// Copy the label's bytes (offset by 1 to skip past the labelLen)
		for (uint8_t i = 0; i < labelLen; i++) {			
			this->nameBytes[this->len + i] = dnsbuffer[this->len + i + 1];
		}

		if (labelLen == 0) {	// It's the root label that marks the end of the name
			if (this->len == 0) {
				// The entire name is the root label, which we represent as " "
				this->nameBytes[this->len] = ' ';
				this->nameBytes[this->len + 1] = '\0';
			} else {
				// We've been adding a series of labels, each followed by '.'
				// Remove the final period
				this->nameBytes[this->len - 1] = '\0';
			} 

			this->len++;
			wellFormed = true;			
			break;
		} else if (! (labelLen == 1 && (uint32_t)(this->len + 1) < size && dnsbuffer[this->len + 1] == '*')) {
			// It's not the root label and it's not a wildcard label, so count it
			this->num_labels++;
		}
		this->nameBytes[this->len + labelLen] = '.';
		this->len += labelLen + 1;
	}

	if(! (this->len <= min(MAX_DNS_NAME_LEN, size) && wellFormed)) {
		Throw(CryptoException::BAD_INPUT, "DomainName badly formatted or too long");
	}
}

DomainName::DomainName(const char* name, const uint32_t size) {
	this->parse(name, size);
}

DomainName::DomainName(const DomainName* name) {
	if (!name) { Throw(CryptoException::BAD_INPUT, "Tried to duplicate a NULL DomainName"); }
	this->parse(name->nameBytes, lite_strlen(name->nameBytes) + 1);
}

// Valid characters for a DNS name (not counting '.')
static bool isLetter(const char c) {
	return ('a' <= c && c <= 'z') ||
		     ('A' <= c && c <= 'Z');
}

static bool isLetterDigit(const char c) {
	return ('a' <= c && c <= 'z') ||
		   ('A' <= c && c <= 'Z') ||
		   ('0' <= c && c <= '9');
}

static bool isLetterDigitHyphen(const char c) {
	return ('a' <= c && c <= 'z') ||
		   ('A' <= c && c <= 'Z') ||
		   ('0' <= c && c <= '9') ||
		    c == '-';
}


/* 
 * Attempts to parse a label from characters [start, start+len) 
 * Looks for [a-zA-Z]([a-zA-Z0-9|-]*[a-zA-Z0-9])*
 * Returns the length of the label found (or -1 if none found)
 */
static int checkLabel(const char* name, uint32_t start, uint32_t len) {
	if (len <= 0) { // Label must include at least one character
		return -1;
	}

	if (!isLetter(name[start])) {  // Must start with a letter
		return -1;
	}

	uint32_t labelLen = 1;
	for (; labelLen < len; labelLen++) {
		if (!isLetterDigitHyphen(name[start+labelLen])) {			
			break;
		}
	}

	// Final character must be a letter or a digit
	if (isLetterDigit(name[start+labelLen-1])) {
		return labelLen;
	} else {
		return -1;
	}
}

/* 
 * Attempts to parse a subdomain from characters [start, start+len) 
 * Returns the length of the subdomain found (or -1 if none found)
 */
static int checkSubdomain(const char* name, uint32_t start, uint32_t len) {
	if (len <= 0) { // Subdomain must include at least one character
		return -1;
	}

	uint8_t labelLen = checkLabel(name, start, len);

	if (labelLen == -1) {
		return -1;
	} else if (labelLen == len) {
		return len;
	} else if (labelLen > len - 2) {
		// No room for a '.' followed by a label
		return -1;
	}

	if (name[start + labelLen] == '.' &&
		checkSubdomain(name, start + labelLen + 1, len - labelLen - 1) != -1) {
		return len;
	} else {
		return -1;
	}

	//if (labelLen != len && /* Label didn't use up the entire subdomain */
	//	len > 3 ) {  /* Form requires at least 3 chars: <subdomain> "." <label> */
	//	uint8_t subdomainLen = this->checkSubdomain(name, start, len - 1);
	//	if (subdomainLen > 0 && name[start + subdomainLen] == '.') {
	//		labelEnd = this->checkLabel(name, subdomainEnd + 2, end);
	//		return labelEnd == end ? end : -1;
	//	} 
	//	return -1;
	//} else {
	//	return end;
	//}	
}


/*
 From RFC 1035: http://tools.ietf.org/html/rfc1035
<domain> ::= <subdomain> | " "
<subdomain> ::= <label> | <label> "." <subdomain>
<label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
<ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
<let-dig-hyp> ::= <let-dig> | "-"
<let-dig> ::= <letter> | <digit>
<letter> ::= any one of the 52 alphabetic characters A through Z in
upper case and a through z in lower case
<digit> ::= any one of the ten digits 0 through 9

Labels must be 63 characters or less.  No significance is attached to the case
*/

bool DomainName::checkDomainName(const char* name, uint32_t len) {
	if (len == 0) {
		return false;
	}

	if (len == 1 && name[0] == ' ') {
		return true;
	}

	return (uint32_t)checkSubdomain(name, 0, len) == len;
}
	
static char toLower(const char c) {
	if ('A' <= c && c <= 'Z') {
		return 'a' + (c - 'A');
	} else {
		return c;
	}
}

// Does the actual work of constructing a Domain Name from a string
void DomainName::parse(const char* name, const uint32_t len) {
	if (!name || !DomainName::checkDomainName(name, len - 1)) { // -1 for Null terminator
		Throw(CryptoException::BAD_INPUT, "Null DomainName"); 
	}

	uint8_t nameLength = lite_strnlen(name, len);
	if (nameLength <= 0 || nameLength > DomainName::MAX_DNS_NAME_LEN) {
		Throw(CryptoException::BAD_INPUT, "Bad length for DomainName");
	} 

	if (nameLength == len && name[nameLength - 1] != '\0') {
		Throw(CryptoException::BAD_INPUT, "DomainName missing a null terminator");
	}

	// Copy over the bytes, converting everything to a canonical lower case form
	for (uint8_t i = 0; i < nameLength+1; i++) {
		this->nameBytes[i] = toLower(name[i]);
	}

	this->num_labels = 0;
	if (nameLength == 1 && name[0] == ' ') {
		this->num_labels = 0; // Root doesn't count
		this->len = 1;
	} else {
		// Count the number of labels
		for (int i = 0; i < nameLength; i++) {
			if (name[i] == '.') {
				this->num_labels++;
			}
		}
		this->num_labels++;	// One for the final label (which doesn't end in a '.')

		// Calculate the size needed to store this in DNS-approved format
		this->len = (nameLength - (this->num_labels - 1))  /* Number of characters in the labels */
					+ this->num_labels				/* One byte to describe the length of each label */
					+ 1;							/* Store the root label of 0 */
	}
}

uint32_t DomainName::size() const {
	return this->len;
}


// Write the domain name into the buffer in DNS form and return number of bytes written
// Assumes buffer is at least size() long
uint32_t DomainName::serialize(uint8_t* buffer) const {
	if(!buffer) { Throw(CryptoException::BAD_INPUT, "Null buffer for serialization"); }

	uint8_t labelPos = 0;
	uint8_t labelLen = 0;
	for (uint8_t i = 0; i < lite_strlen(this->nameBytes); i++) {
		if (this->nameBytes[i] == '.') {
			buffer[labelPos] = labelLen;
			labelLen = 0;
			labelPos = i + 1;
		} else if (this->nameBytes[i] == ' ') { // This is the root label
			buffer[i] = 0;
		} else {
			buffer[i + 1] = toLower(this->nameBytes[i]);  // + 1 to allow room for the labelLen
			labelLen++;
		}
	}
	// Write out the final label length
	buffer[labelPos] = labelLen;

	// Finish with the root label
	buffer[this->len - 1] = 0;

	return this->len;
}


// Write the domain name into the buffer in DNS form and return number of bytes written
// Assumes buffer is at least size() long
uint32_t DomainName::serialize(ByteStream &buffer) const {
	uint8_t labelPos = 0;
	uint8_t labelLen = 0;
	for (uint8_t i = 0; i < lite_strlen(this->nameBytes); i++) {
		if (this->nameBytes[i] == '.') {
			buffer[labelPos] = labelLen;
			labelLen = 0;
			labelPos = i + 1;
		} else if (this->nameBytes[i] == ' ') { // This is the root label
			buffer[i] = 0;
		} else {
			buffer[i + 1] = toLower(this->nameBytes[i]);  // + 1 to allow room for the labelLen
			labelLen++;
		}
	}
	// Write out the final label length
	buffer[labelPos] = labelLen;

	// Finish with the root label
	buffer[this->len - 1] = 0;

	return this->len;
}


// Return true if this DomainName has name for a suffix
bool DomainName::suffix(const DomainName* parent) const {
	if (!parent || !(parent->len <= this->len)) {
		return false;
	}

	if (parent->len == 1) { // Root is a suffix for everything
		return true;
	}
	
	// Compare characters starting at the end and moving forward.  All must match
	int parentPos = lite_strlen(parent->nameBytes) - 1;
	int pos = lite_strlen(this->nameBytes) - 1;

	while (parentPos >= 0) {
		if (this->nameBytes[pos] != parent->nameBytes[parentPos]) {
			return false;
		}
		parentPos--;
		pos--;
	}
	
	// All the characters match.
	// Still need to make sure we're at the end of the child's characters
	// or at the end of a label.
	// Otherwise, we'd accept bemicrosoft.com as a suffix of microsoft.com
	if (! (pos < 0  /* We reached the end of child at the same time as parent */
			|| this->nameBytes[pos] == '.')) { /* We're at the end of a child label */ 
		return false;
	}
	
	return true;
}

// Number of labels, not counting the root or any wildcards
uint8_t DomainName::numLabels() const {
	return this->num_labels;
}

bool operator==(DomainName &d1, DomainName &d2) {
	if (d1.len != d2.len || d1.num_labels != d2.num_labels) {
		return false;
	}

	for (uint32_t i = 0; i < lite_strlen(d1.nameBytes); i++) {
		if (d1.nameBytes[i] != d2.nameBytes[i]) {
			return false;
		}
	}

	return true;
}

bool operator!=(DomainName &d1, DomainName &d2) { 
	return !(d1 == d2);
}

char* DomainName::toString() {
	return this->nameBytes;
}
