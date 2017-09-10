#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <locale>
#include <regex>
#include <functional>
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
#define EMAIL_VALIDATION_SLEEP_TIME 5000
#define MAX_EMAIL_VALIDATION_REATTEMPTS 5


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
    function<void()> Action = bind(&Account::CreateYuisyAccount, this);

    struct Status {
      enum class Code {
        kStartingAccountCreation,
        kCreatingYuisyAccount,
        kYuisyAccountCreated,
        kNicknameAndEmailTaken,
        kNicknameTaken,
        kEmailTaken,
        kRToCreateYuisyAccount,
        kMaxReattemptsExceeced,
        kRequestingAndCheckingTemporaryEmailAddress,
        kEmailValidationLinkFound,
        kEmailValidationLinkNotFound,
        kRToFindEmailValidationLink,
        kRToRequestAndCheckTemporaryEmailAddress,
        kValidatingTemporaryEmailAddress,
        kRToValidateTemporaryEmailAddress,
        kSuccess,
        kFail
      };

      bool success = false;
    } status;

    inline void SetEmail();
    void PrintStatus(Status::Code, optional<int> = nullopt);
    void CreateYuisyAccount();
    void RequestAndCheckTemporaryEmailAddress();
    void ValidateTemporaryEmailAddress(string);
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

vector<string> RequestDomainsList() // falta pulir
{
  cout << "> Obteniendo lista de dominios para los emails temporales..." << endl;

  CURL *curl = curl_easy_init();
  vector<string> domains_list;

  if (curl) {
    string json;
    curl_easy_setopt(curl, CURLOPT_URL, "http://api.temp-mail.ru/request/domains/format/json/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);

    CURLcode res;
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      cerr << "> No se ha podido obtener la lista." << endl;
      system("PAUSE");
      exit(EXIT_FAILURE);
    }

    rapidjson::Document document;
    const rapidjson::Value &kDomainsList = document.Parse(json);

    for (rapidjson::SizeType i = 0; i < kDomainsList.Size(); ++i)
      domains_list.push_back(kDomainsList[i].GetString());

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
  *const_cast<string *> (&kDomain) = kDomainsList[/*(rand() % kDomainsList.size())*/0]; // bug?
}

void Account::Create()
{
  SetEmail();
  PrintStatus(Status::Code::kStartingAccountCreation);

  do {
    Action();
  } while (Action);

  status.success ? PrintStatus(Status::Code::kSuccess) : PrintStatus(Status::Code::kFail);
}

void Account::SetEmail()
{
  *const_cast<string *> (&kEmail) = (kName.value() + kDomain);
}

void Account::PrintStatus(const Status::Code kStatusCode, const optional<int> kReattempt)
{
  switch (kStatusCode) {
    case Status::Code::kStartingAccountCreation: {
      fmt::print(" Nombre: {}.\n", kName.value());
      break;
    }

    case Status::Code::kCreatingYuisyAccount: {
      cout << "  - Creando cuenta de Yuisy..." << endl;
      break;
    }

    case Status::Code::kYuisyAccountCreated: {
      cout << "  - Se ha creado la cuenta de Yuisy." << endl;
      break;
    }

    case Status::Code::kNicknameAndEmailTaken: {
      cout << "  - El nombre y el email ya están registrados." << endl;
      break;
    }

    case Status::Code::kNicknameTaken: {
      cout << "  - El nombre ya está registrado." << endl;
      break;
    }

    case Status::Code::kEmailTaken: {
      cout << "  - El email ya está registrado." << endl;
      break;
    }

    case Status::Code::kRToCreateYuisyAccount: {
      fmt::print("  - No se pudo crear la cuenta de Yuisy. Reintentando ({}/{})...\n",
        kReattempt.value(),
        MAX_REATTEMPTS);

      break;
    }

    case Status::Code::kMaxReattemptsExceeced: {
      cout << "  - Se han excedido los reintentos máximos." << endl;
      break;
    }

    case Status::Code::kRequestingAndCheckingTemporaryEmailAddress: {
      cout << "  - Buscando enlace de confirmación de email en la bandeja de entrada..." << endl;
      break;
    }

    case Status::Code::kEmailValidationLinkFound: {
      cout << "  - Se ha encontrado el enlace." << endl;
      break;
    }

    case Status::Code::kEmailValidationLinkNotFound: {
      fmt::print("  - No se ha encontrado el enlace. Reintentando en {} milisegundos.\n",
        EMAIL_VALIDATION_SLEEP_TIME);

      break;
    }

    case Status::Code::kRToFindEmailValidationLink: {
      fmt::print("  - Reintentando encontrar enlace ({}/{})...\n",
        kReattempt.value(),
        MAX_EMAIL_VALIDATION_REATTEMPTS);

      break;
    }

    case Status::Code::kRToRequestAndCheckTemporaryEmailAddress: {
      fmt::print("  - No se pudo abrir la bandeja de entrada. Reintentando ({}/{})...\n",
        kReattempt.value(),
        MAX_REATTEMPTS);

      break;
    }

    case Status::Code::kSuccess: {
      cout << "  - La cuenta está lista para ser usada." << endl;
      break;
    }

    case Status::Code::kFail: {
      cout << "  - No se pudo crear la cuenta." << endl;
    }
  }
}

void Account::CreateYuisyAccount()
{
  PrintStatus(Status::Code::kCreatingYuisyAccount);
  CURL *curl = curl_easy_init();

  if (curl) {
    const string kPostFields = fmt::format("apodo={0}&email={1}&confirmar_email={1}&password={2}"
      "&pais={3}&terminos-condiciones=1&form_id=yuisy_login_register_form",
      kName.value(),
      kEmail,
      kPassword,
      COUNTRY_ID);

    string html;
    curl_easy_setopt(curl, CURLOPT_URL, "http://yuisy.com/");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, kPostFields.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

    CURLcode res;
    int reattempts = 0;

    do {
      if (!html.empty()) html.clear();
      res = curl_easy_perform(curl);

      if (res == CURLE_OK) {
        long http_status_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);

        if (http_status_code == 200 || http_status_code == 302) {
          if (html.find("valida tu e-mail") != string::npos) {
            PrintStatus(Status::Code::kYuisyAccountCreated);
            Action = bind(&Account::RequestAndCheckTemporaryEmailAddress, this);
            break;
          } else {
            const string kFormatStr = "El {} <strong>{}</strong>";
            const string kNicknameStr = fmt::format(kFormatStr, "apodo", kName.value());
            const string kEmailStr = fmt::format(kFormatStr, "email", kEmail);
            const bool kIsNicknameTaken = html.find(kNicknameStr) != string::npos;
            const bool kIsEmailTaken = html.find(kEmailStr) != string::npos;

            if (kIsNicknameTaken || kIsEmailTaken) {
              if (kIsNicknameTaken && kIsEmailTaken) {
                PrintStatus(Status::Code::kNicknameAndEmailTaken);
              } else if (kIsNicknameTaken) {
                PrintStatus(Status::Code::kNicknameTaken);
              } else {
                PrintStatus(Status::Code::kEmailTaken);
              }

              Action = nullptr;
              break;
            }
          }
        }
      }

      reattempts++;

      if (reattempts <= MAX_REATTEMPTS) {
        PrintStatus(Status::Code::kRToCreateYuisyAccount, reattempts);
      } else {
        PrintStatus(Status::Code::kMaxReattemptsExceeced);
        Action = nullptr;
      }
    } while (reattempts <= MAX_REATTEMPTS);

    curl_easy_cleanup(curl);
  }
}

void Account::RequestAndCheckTemporaryEmailAddress()
{
  PrintStatus(Status::Code::kRequestingAndCheckingTemporaryEmailAddress);
  CURL *curl = curl_easy_init();

  if (curl) {
    hashwrapper *md5_wrapper = new md5wrapper();

    const string kUrl = fmt::format("http://api.temp-mail.ru/request/mail/id/{}/format/json/",
      md5_wrapper->getHashFromString(kEmail));

    delete md5_wrapper;
    string json;
    curl_easy_setopt(curl, CURLOPT_URL, kUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);

    CURLcode res;
    int reattempts = 0;
    int email_validation_reattempts = 0;

    do {
      if (!json.empty()) json.clear();
      res = curl_easy_perform(curl);

      if (res == CURLE_OK) {
        long http_status_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);

        if (http_status_code == 200) {
          PrintStatus(Status::Code::kEmailValidationLinkFound);

          rapidjson::Document document;
          document.Parse(json);

          string mail_text = document[0]["mail_text"].GetString();
          mail_text.pop_back();

          Action = bind(&Account::ValidateTemporaryEmailAddress, this,
            mail_text.substr(mail_text.rfind("http")));

          break;
        } else if (http_status_code == 404) {
          if (email_validation_reattempts < MAX_EMAIL_VALIDATION_REATTEMPTS) {
            PrintStatus(Status::Code::kEmailValidationLinkNotFound);
            Sleep(EMAIL_VALIDATION_SLEEP_TIME);
            email_validation_reattempts++;
            PrintStatus(Status::Code::kRToFindEmailValidationLink, email_validation_reattempts);
            continue;
          }

          email_validation_reattempts = 0;
        } else if (http_status_code == 429) {
          continue;
        }
      }

      reattempts++;

      if (reattempts <= MAX_REATTEMPTS) {
        PrintStatus(Status::Code::kRToRequestAndCheckTemporaryEmailAddress, reattempts);
      } else {
        PrintStatus(Status::Code::kMaxReattemptsExceeced);
        Action = nullptr;
      }
    } while (reattempts <= MAX_REATTEMPTS);

    curl_easy_cleanup(curl);
  }
}

void Account::ValidateTemporaryEmailAddress(const string kEmailValidationLink) // casi listo
{
  PrintStatus(Status::Code::kValidatingTemporaryEmailAddress);
  CURL *curl = curl_easy_init();

  if (curl) {
    string html;
    curl_easy_setopt(curl, CURLOPT_URL, kEmailValidationLink.c_str());
    //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

    CURLcode res;
    int reattempts = 0;

    do {
      if (!html.empty()) html.clear();
      res = curl_easy_perform(curl);

      if (res == CURLE_OK) {
        long http_status_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
      }

      reattempts++;

      if (reattempts <= MAX_REATTEMPTS) {
        PrintStatus(Status::Code::kRToValidateTemporaryEmailAddress, reattempts);
      } else {
        PrintStatus(Status::Code::kMaxReattemptsExceeced);
        Action = nullptr;
      }
    } while (reattempts <= MAX_REATTEMPTS);
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
