#include <iostream>
#include <locale>
#include <string>
#include <vector>
#include <limits>
#include <regex>
#include <optional>
#include <curl/curl.h>
#include <hl_md5wrapper.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>

#define MAX_QUANTITY 100
#define MIN_NAME_LENGTH 3
#define MAX_NAME_LENGTH 12
#define MAX_BASENAME_LENGTH (MAX_NAME_LENGTH - 3)
#define MIN_PASSWORD_LENGTH 6
#define MAX_PASSWORD_LENGTH 16

using namespace std;

size_t WriteFunction(char *ptr, size_t size, size_t nmemb, string *userdata);
vector<string> RequestDomainsList();
int AskQuantity();
string AskName(bool);
string AskPassword(bool);
bool ShouldRestart();

class Account {
public:
  Account(optional<string> name, string email, string password) : kName(name), kEmail(email), kPassword(password) {}
  virtual void Create();
  void CreateYuisyAccount();
  void RequestAndCheckTemporaryEmailAddress();
protected:
  const optional<string> kName;
  const string kEmail;
  const string kPassword;
};

class MultiAccount : public Account {
public:
  MultiAccount(string email, string password, int ID) : Account(nullopt, email, password), kID(ID) {}
  inline static void SetBaseName(string);
  void Create() override;
  void SetName();
private:
  const int kID;
  static string kBaseName;
};

string MultiAccount::kBaseName;

int main()
{
  locale::global(locale("spanish"));

  cout << "# Yuisy Account Creator" << endl;
  cout << "# Hecho por Mars.-" << endl;
  cout << "# Reportar problemas al siguiente correo: everythingispermitted@outlook.com" << endl << endl;

  curl_global_init(CURL_GLOBAL_ALL);

  const vector<string> kDomainsList = RequestDomainsList();

  do {
    const unsigned kQuantity = AskQuantity();

    if (kQuantity == 1) {
      Account account(AskName(false), kDomainsList[(rand() % kDomainsList.size())], AskPassword(false));
      account.Create();
    } else {
      MultiAccount::SetBaseName(AskName(true));
      const string kPassword = AskPassword(true);
      vector<MultiAccount *> accounts;

      for (unsigned i = 0; i < kQuantity; ++i) {
        MultiAccount *account = new MultiAccount(kDomainsList[(rand() % kDomainsList.size())], kPassword, (i + 1));
        account->Create();
        accounts.push_back(account);
      }
      // ...
      for (vector<MultiAccount *>::iterator it = accounts.begin(); it != accounts.end(); ++it) {
        delete *it;
      }
    }
  } while (ShouldRestart());

  curl_global_cleanup();
  return 0;
}

size_t WriteFunction(char *ptr, size_t size, size_t nmemb, string *userdata)
{
  size_t real_size = (size * nmemb);
  userdata->append(ptr, real_size);
  return real_size;
}

vector<string> RequestDomainsList()
{
  cout << "> Obteniendo lista de dominios para los emails temporales..." << endl;

  CURL *curl = curl_easy_init();
  vector<string> domains_list;

  if (curl) {
    CURLcode res;
    string json;
    curl_easy_setopt(curl, CURLOPT_URL, "http://api.temp-mail.ru/request/domains/format/json/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      cout << "> No se pudo obtener la lista" << endl;
      system("PAUSE");
      exit(EXIT_FAILURE);
    }

    rapidjson::Document document;
    document.Parse(json);
    const rapidjson::Value &kJsonArray = document;

    for (rapidjson::SizeType i = 0; i < kJsonArray.Size(); ++i)
      domains_list.push_back(kJsonArray[i].GetString());

    curl_easy_cleanup(curl);
  }

  return domains_list;
}

int AskQuantity()
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "< Ingresa la cantidad de cuentas que quieres crear";
      cout << (" (Debe ser entre 1 y " + to_string(MAX_QUANTITY) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "> Cantidad inválida" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([1-9]([0-9]{1,2})?)$")) || (stoi(answer) > MAX_QUANTITY));

  return stoi(answer);
}

string AskName(const bool kMultiple)
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "< Ingresa un " << (kMultiple ? "nombre base para las cuentas" : "nombre para la cuenta");
      cout << (" (Debe contener sólo letras y una longitud entre "
        + to_string(MIN_NAME_LENGTH) + " y " + to_string(kMultiple ? MAX_BASENAME_LENGTH : MAX_NAME_LENGTH) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "> " << (kMultiple ? "Nombre base" : "Nombre") << " inválido" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([a-z]{"
    + to_string(MIN_NAME_LENGTH) + "," + to_string(kMultiple ? MAX_BASENAME_LENGTH : MAX_NAME_LENGTH) + "})$", regex_constants::icase)));

  return answer;
}

string AskPassword(const bool kMultiple)
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "< Ingresa una contrasenya para " << (kMultiple ? "las cuentas" : "la cuenta");
      cout << (" (Debe contener sólo letras y numeros, y una longitud entre "
        + to_string(MIN_PASSWORD_LENGTH) + " y " + to_string(MAX_PASSWORD_LENGTH) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "Contrasenya inválida" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([a-z0-9]{"
    + to_string(MIN_PASSWORD_LENGTH) + "," + to_string(MAX_PASSWORD_LENGTH) + "})$", regex_constants::icase)));

  return answer;
}

bool ShouldRestart()
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "< ¿Crear más cuentas?" << endl;
      first_attempt = false;
    } else {
      cout << "> Responde si/s o no/n" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^((si|s)|(no|n))$", regex_constants::icase)));

  return regex_match(answer, regex("^(si|s)$", regex_constants::icase)) ? true : false;
}

void Account::Create()
{
  cout << ("Creando la siguiente cuenta: " + kName.value()) << endl;

  CreateYuisyAccount();
  RequestAndCheckTemporaryEmailAddress();
}

void Account::CreateYuisyAccount()
{
  // ...
}

void Account::RequestAndCheckTemporaryEmailAddress() // ...
{
  CURL *curl = curl_easy_init();

  if (curl) {
    hashwrapper *md5_wrapper = new md5wrapper();
    CURLcode res;
    string json;
    curl_easy_setopt(curl, CURLOPT_URL, ("http://api.temp-mail.ru/request/mail/id/" + md5_wrapper->getHashFromString(kEmail) + "/format/json/"));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
    }

    curl_easy_cleanup(curl);
    delete md5_wrapper;
  }
}

void MultiAccount::SetBaseName(const string kTargetBaseName)
{
  kBaseName = kTargetBaseName;
}

void MultiAccount::Create()
{
  SetName();
  Account::Create();
}

void MultiAccount::SetName()
{
  optional<string> *name = const_cast<optional<string> *> (&kName);

  if (kID < 10) {
    *name = (kBaseName + "00" + to_string(kID));
  } else if (kID < 100) {
    *name = (kBaseName + "0" + to_string(kID));
  } else {
    *name = (kBaseName + to_string(kID));
  }
}
