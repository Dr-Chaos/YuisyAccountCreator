#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <locale>
#include <regex>
#include <fmt/format.h>
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
#define COUNTRY_ID 6
#define MAX_REATTEMPTS 3


using namespace std;


size_t WriteFunction(char *ptr, size_t size, size_t nmemb, string *userdata);

vector<string> RequestDomainsList();

int AskQuantity();

string AskName(bool);

string AskPassword(bool);

bool ShouldRestart();


class Account {
  public:
    Account(optional<string> name, string password) : kName(name), kPassword(password) {}
    inline static void SetDomain(vector<string>);
    virtual void Create();
  protected:
    const optional<string> kName;
  private:
    const static string kDomain;
    const string kEmail;
    const string kPassword;

    struct Status {
      enum class Code {
        kSuccess,
        kErrorAt1,
        kErrorAt2
      } code;

      string text;
      int reattempts;
    } status;

    inline void SetEmail();
    void CreateYuisyAccount();
    void RequestAndCheckTemporaryEmailAddress();
    void Reattempt(Status::Code);
};

const string Account::kDomain;

class MultiAccount : public Account {
  public:
    MultiAccount(string password, int ID) : Account(nullopt, password), kID(ID) {}
    inline static void SetBaseName(string);
    void Create() override;
  private:
    const static string kBaseName;
    const int kID;

    void SetName();
};

const string MultiAccount::kBaseName;


int main()
{
  locale::global(locale("spanish"));

  cout << "# Yuisy Account Creator" << endl << "# Hecho por Mars.-" << endl << endl;

  curl_global_init(CURL_GLOBAL_ALL);
  Account::SetDomain(RequestDomainsList());

  do {
    const unsigned kQuantity = AskQuantity();

    if (kQuantity == 1) {
      const string kName = AskName(false);
      Account account(kName, AskPassword(false));
      account.Create();
    } else {
      MultiAccount::SetBaseName(AskName(true));
      const string kPassword = AskPassword(true);
      vector<MultiAccount *> accounts;

      for (unsigned i = 0; i < kQuantity; ++i) {
        MultiAccount *account = new MultiAccount(kPassword, (i + 1));
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
      cerr << "> No se ha podido obtener la lista." << endl;
      system("PAUSE");
      exit(EXIT_FAILURE);
    }

    rapidjson::Document document;
    const rapidjson::Value &kJsonArray = document.Parse(json);

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
      fmt::print("< Ingresa la cantidad de cuentas que quieres crear (Debe ser un número entre 1 y"
        " {}).\n",
        MAX_QUANTITY);

      first_attempt = false;
    } else {
      cout << "> Cantidad inválida." << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([1-9]([0-9]{1,2})?)$")) || (stoi(answer) > MAX_QUANTITY));

  return stoi(answer);
}

string AskName(const bool kMultiple)
{
  bool first_attempt = true;
  string answer;

  const string kRegex = fmt::format("^([a-z]{{{},{}}})$",
    MIN_NAME_LENGTH,
    (!kMultiple ? MAX_NAME_LENGTH : MAX_BASENAME_LENGTH));

  do {
    if (first_attempt) {
      fmt::print("< Ingresa un nombre para {} (Debe contener sólo letras y una longitud entre {} y"
        " {}).\n",
        (!kMultiple ? "la cuenta" : "las cuentas"),
        MIN_NAME_LENGTH,
        (!kMultiple ? MAX_NAME_LENGTH : MAX_BASENAME_LENGTH));

      first_attempt = false;
    } else {
      cout << "> Nombre inválido." << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex(kRegex, regex_constants::icase)));

  return answer;
}

string AskPassword(const bool kMultiple)
{
  bool first_attempt = true;
  string answer;

  const string kRegex = fmt::format("^([a-z0-9]{{{},{}}})$",
    MIN_PASSWORD_LENGTH,
    MAX_PASSWORD_LENGTH);

  do {
    if (first_attempt) {
      fmt::print("< Ingresa una contraseña para {} (Debe contener sólo letras y números, y una "
        "longitud entre {} y {}).\n",
        (!kMultiple ? "la cuenta" : "las cuentas"),
        MIN_PASSWORD_LENGTH,
        MAX_PASSWORD_LENGTH);

      first_attempt = false;
    } else {
      cout << "> Contraseña inválida." << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex(kRegex, regex_constants::icase)));

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
      cout << "> Responde si/s o no/n." << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^((si|s)|(no|n))$", regex_constants::icase)));

  return regex_match(answer, regex("^(si|s)$", regex_constants::icase)) ? true : false;
}

void Account::SetDomain(const vector<string> kDomainsList)
{
  *const_cast<string *> (&kDomain) = kDomainsList[(rand() % kDomainsList.size())];
}

void Account::Create()
{
  SetEmail();
  CreateYuisyAccount();
  RequestAndCheckTemporaryEmailAddress();
}

void Account::SetEmail()
{
  *const_cast<string *> (&kEmail) = (kName.value() + kDomain);
}

void Account::CreateYuisyAccount()
{
  CURL *curl = curl_easy_init();

  if (curl) {
    const string kPostFields = fmt::format("apodo={0}&email={1}&confirmar_email={1}&password={2}"
      "&pais={3}&terminos-condiciones=1&form_id=yuisy_login_register_form",
      kName.value(),
      kEmail,
      kPassword,
      COUNTRY_ID);

    string html;
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, "http://yuisy.com/");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, kPostFields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      long http_status_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
      curl_easy_cleanup(curl);

      if (http_status_code == 200) {
        if (html.find("valida tu e-mail") != string::npos) {
          //
        } else {
          const string format_str = "el {} {} ya está registrado";

          if (html.find(fmt::format(format_str, "apodo", kName.value())) != string::npos) {
            //
          } else if (html.find(fmt::format(format_str, "email", kEmail)) != string::npos) {
            //
          } else {
            //
          }
        }
      } else {
        Reattempt(Status::Code::kErrorAt2);
      }
    } else {
      curl_easy_cleanup(curl);
      Reattempt(Status::Code::kErrorAt1);
    }
  }
}

void Account::RequestAndCheckTemporaryEmailAddress() // ...
{
  CURL *curl = curl_easy_init();

  if (curl) {
    hashwrapper *md5_wrapper = new md5wrapper();

    const string kUrl = fmt::format("http://api.temp-mail.ru/request/mail/id/{}/format/json/",
      md5_wrapper->getHashFromString(kEmail));

    string json;
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, kUrl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
    }

    curl_easy_cleanup(curl);
    delete md5_wrapper;
  }
}

void Account::Reattempt(const Status::Code kErrorCode)
{
  if (kErrorCode == status.code) {
    if (status.reattempts == MAX_REATTEMPTS) {
      return;
    }

    status.reattempts++;
  } else {
    status.code = kErrorCode;
    status.reattempts = 1;
  }

  switch (kErrorCode) {
    case Status::Code::kErrorAt1: {
      CreateYuisyAccount();
      break;
    }

    case Status::Code::kErrorAt2: {
      RequestAndCheckTemporaryEmailAddress();
    }
  }
}

void MultiAccount::SetBaseName(const string kTargetBaseName)
{
  *const_cast<string *> (&kBaseName) = kTargetBaseName;
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
