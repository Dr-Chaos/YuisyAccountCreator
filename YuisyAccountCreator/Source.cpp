#include <iostream>
#include <locale>
#include <string>
#include <vector>
#include <limits>
#include <regex>
#include "curl/curl.h"
#include "rapidjson/document.h"

#define MAX_QUANTITY 100
#define MIN_NAME_LENGTH 3
#define MAX_NAME_LENGTH 12
#define MAX_BASENAME_LENGTH (MAX_NAME_LENGTH - 3)
#define MIN_PASSWORD_LENGTH 6
#define MAX_PASSWORD_LENGTH 16

using namespace std;
using namespace rapidjson;

vector<string> RequestDomainsList();
int AskQuantity();
string AskName();
string AskBasename();
string AskPassword();

class Account {
public:
  void SetEmail(string);
  void RequestTemporaryEmailAddress();
protected:
  string name;
  string email;
};

class SingleAccount : public Account {
public:
  SingleAccount(const string Name) { name = Name; }
};

class MultiAccount : public Account {
public:
  MultiAccount(const int Id) : id(Id) {}
  void SetName(string);
private:
  const int id;
};

int main()
{
  locale::global(locale("spanish"));
  curl_global_init(CURL_GLOBAL_ALL);

  string domain = RequestDomainsList()[0];
  bool exit = true;

  do {
    const unsigned quantity = AskQuantity();

    if (quantity == 1) {
      const string name = AskName();
      SingleAccount account(name);
      account.SetEmail(domain);
      account.RequestTemporaryEmailAddress();
    } else {
      const string base_name = AskBasename();
      vector<MultiAccount *> accounts;

      for (unsigned i = 0; i < quantity; ++i) {
        const int id = (i + 1);
        MultiAccount *account = new MultiAccount(id);
        account->SetName(base_name);
        account->SetEmail(domain);
        account->RequestTemporaryEmailAddress();
        accounts.push_back(account);
      }
      // ...
      for (vector<MultiAccount *>::iterator it = accounts.begin(); it != accounts.end(); ++it) {
        delete *it;
      }
    }
  } while (!exit);

  curl_global_cleanup();
  return 0;
}

vector<string> RequestDomainsList()
{
  CURL *curl = curl_easy_init();
  CURLcode res;
  Document document;
  vector<string> domains;

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://api.temp-mail.ru/request/domains/format/json/");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "a=1&b=2");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, /**/);
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      //
    } else {
      document.Parse(res);
    }

    curl_easy_cleanup(curl);
  }

  return domains;
}

int AskQuantity()
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "Ingresa la cantidad de cuentas que quieres crear";
      cout << (" (Debe ser entre 1 y " + to_string(MAX_QUANTITY) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "Cantidad inválida" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([1-9]([0-9]{1,2})?)$")) || (stoi(answer) > MAX_QUANTITY));

  return stoi(answer);
}

string AskName()
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "Ingresa un nombre para la cuenta";
      cout << (" (Debe contener sólo letras y una longitud entre "
        + to_string(MIN_NAME_LENGTH) + " y " + to_string(MAX_NAME_LENGTH) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "Nombre inválido" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([a-zA-Z]{" + to_string(MIN_NAME_LENGTH) + "," + to_string(MAX_NAME_LENGTH) + "})$")));

  return answer;
}

string AskBasename()
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "Ingresa un nombre base para las cuentas";
      cout << (" (Debe contener sólo letras y una longitud entre "
        + to_string(MIN_NAME_LENGTH) + " y " + to_string(MAX_BASENAME_LENGTH) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "Nombre base inválido" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([a-zA-Z]{" + to_string(MIN_NAME_LENGTH) + "," + to_string(MAX_BASENAME_LENGTH) + "})$")));

  return answer;
}

string AskPassword()
{
  string t;
  return t;
}

void Account::SetEmail(const string domain)
{
  email = (name + "@" + domain);
}

void Account::RequestTemporaryEmailAddress()
{
}

void MultiAccount::SetName(const string base_name)
{
  if (id < 10) {
    name = (base_name + "00" + to_string(id));
  } else if (id < 100) {
    name = (base_name + "0" + to_string(id));
  } else {
    name = (base_name + to_string(id));
  }
}
