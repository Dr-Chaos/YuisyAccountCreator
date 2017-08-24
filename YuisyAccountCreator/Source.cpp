#include <iostream>
#include <locale>
#include <string>
#include <vector>
#include <limits>
#include <regex>
#include <curl/curl.h>

#define MAX_QUANTITY 100
#define MIN_NAME_LENGTH 3
#define MAX_NAME_LENGTH 12
#define MAX_BASE_NAME_LENGTH (MAX_NAME_LENGTH - 3)
#define MIN_PASSWORD_LENGTH 6
#define MAX_PASSWORD_LENGTH 16

using namespace std;

vector<string> RequestDomainsList();
int AskQuantity();
string AskName();
string AskBaseName();
string AskPassword();

class Account {
public:
  void SetEmail(const string);
  void RequestTemporaryEmailAddress();
protected:
  string name;
  string email;
};

class SingleAccount : public Account {
public:
  void SetName(const string _name) { name = _name; }
};

class MultiAccount : public Account {
public:
  MultiAccount(const int _ID) : ID(_ID) {}
  void SetName(const string);
private:
  const int ID;
};

int main()
{
  locale::global(locale("spanish"));
  bool exit = true;
  const string domain = RequestDomainsList()[0];

  do {
    const unsigned quantity = AskQuantity();

    if (quantity == 1) {
      const string name = AskName();
      SingleAccount account;
      account.SetName(name);
      account.SetEmail(domain);
      account.RequestTemporaryEmailAddress();
    } else {
      const string base_name = AskBaseName();
      vector<MultiAccount *> accounts;

      for (unsigned i = 0; i < quantity; ++i) {
        const int ID = (i + 1);
        MultiAccount *account = new MultiAccount(ID);
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

  system("PAUSE");
  return 0;
}

vector<string> RequestDomainsList()
{
  cout << "Obteniendo lista de dominios..." << endl;
  vector<string> t;
  t.push_back("123");
  return t;
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

string AskBaseName()
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "Ingresa un nombre base para las cuentas";
      cout << (" (Debe contener sólo letras y una longitud entre "
        + to_string(MIN_NAME_LENGTH) + " y " + to_string(MAX_BASE_NAME_LENGTH) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "Nombre base inválido" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([a-zA-Z]{" + to_string(MIN_NAME_LENGTH) + "," + to_string(MAX_BASE_NAME_LENGTH) + "})$")));

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
  if (ID < 10) {
    name = (base_name + "00" + to_string(ID));
  } else if (ID < 100) {
    name = (base_name + "0" + to_string(ID));
  } else {
    name = (base_name + to_string(ID));
  }
}
