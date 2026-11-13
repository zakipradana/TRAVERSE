#include "traverse.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>

using namespace std;

Kategori* kepala = nullptr;
int idTerakhir = 0;

namespace {

int KAPASITAS[JUMLAH_JENIS] = {7, 15, 30, 55};
string NAMA_JENIS[JUMLAH_JENIS] = {"Mobil Pribadi", "Travel", "Minibus", "Bigbus"};

// Model adalah contoh. Sesuaikan dengan armada perusahaan Anda.
string MODEL[JUMLAH_JENIS][4] = {
    {"Avanza", "Innova", "Luxio", "Grandmax"},
    {"Hiace Commuter", "Elf", "", ""},
    {"Minibus Tipe A", "Minibus Tipe B", "", ""},
    {"Bigbus Tipe A", "Bigbus Tipe B", "", ""}
};
int JUMLAH_MODEL[JUMLAH_JENIS] = {4, 2, 2, 2};

// Item sementara untuk daftar yang diurutkan (Daftar wisata terurut dan Statistik)
struct Item {
    string label;
    string nama;
    int nilai;
    Item* next;
};

string buatKode(int id) {
    string angka = to_string(id);
    while (angka.size() < 4) angka = "0" + angka;
    return "TRV-" + angka;
}

string formatTanggal(Tanggal t) {
    string bulan = to_string(t.bulan);
    if (bulan.size() < 2) bulan = "0" + bulan;
    string hari = to_string(t.hari);
    if (hari.size() < 2) hari = "0" + hari;
    return to_string(t.tahun) + "-" + bulan + "-" + hari;
}

Tanggal hariIni() {
    time_t now = time(nullptr);
    tm* lt = localtime(&now);
    Tanggal t;
    t.tahun = lt->tm_year + 1900;
    t.bulan = lt->tm_mon + 1;
    t.hari = lt->tm_mday;
    return t;
}

// Jumlah hari dari "dari" ke "ke". Nilai negatif berarti tanggal sudah lewat.
int selisihHari(Tanggal dari, Tanggal ke) {
    tm a = {}, b = {};
    a.tm_year = dari.tahun - 1900; a.tm_mon = dari.bulan - 1; a.tm_mday = dari.hari; a.tm_hour = 12;
    b.tm_year = ke.tahun - 1900;   b.tm_mon = ke.bulan - 1;   b.tm_mday = ke.hari;   b.tm_hour = 12;
    double detik = difftime(mktime(&b), mktime(&a));
    return static_cast<int>(round(detik / 86400.0));
}

void tambahCatatan(Pemesanan* p, string teks) {
    Catatan* baru = new Catatan{teks, nullptr};
    if (p->riwayat == nullptr) {
        p->riwayat = baru;
        return;
    }
    Catatan* q = p->riwayat;
    while (q->next != nullptr) q = q->next;
    q->next = baru;
}

void hapusSemuaCatatan(Catatan* c) {
    while (c) {
        Catatan* nx = c->next;
        delete c;
        c = nx;
    }
}

void hapusSemuaPemesanan(Pemesanan* p) {
    while (p) {
        Pemesanan* nx = p->next;
        hapusDaftarPenumpang(p->daftarPenumpang);
        hapusSemuaCatatan(p->riwayat);
        delete p;
        p = nx;
    }
}

// Total kursi dari anggota saja (tanpa pemesan)
int kursiDaftar(Penumpang* kepala) {
    int total = 0;
    for (Penumpang* q = kepala; q != nullptr; q = q->next) {
        total += kursiAnggota(q->data);
    }
    return total;
}

int hitungAktif(TempatWisata* t) {
    int aktif = 0;
    for (Pemesanan* p = t->daftarPemesanan; p != nullptr; p = p->next) {
        if (p->status != BATAL) aktif++;
    }
    return aktif;
}

// Urutan nama tempat, lalu label
bool sebelumNama(Item* a, Item* b) {
    if (a->nama != b->nama) return a->nama < b->nama;
    return a->label < b->label;
}

// Urutan jumlah terbanyak dulu, lalu label
bool sebelumNilai(Item* a, Item* b) {
    if (a->nilai != b->nilai) return a->nilai > b->nilai;
    return a->label < b->label;
}

void sisipUrut(Item*& kepala, Item* baru, bool (*sebelum)(Item*, Item*)) {
    if (kepala == nullptr || sebelum(baru, kepala)) {
        baru->next = kepala;
        kepala = baru;
        return;
    }
    Item* p = kepala;
    while (p->next != nullptr && !sebelum(baru, p->next)) p = p->next;
    baru->next = p->next;
    p->next = baru;
}

void hapusDaftarItem(Item* k) {
    while (k) {
        Item* nx = k->next;
        delete k;
        k = nx;
    }
}

}  // namespace

// ===== Fungsi bantu =====

int kapasitasJenis(int jenis) {
    return KAPASITAS[jenis];
}

string namaJenis(int jenis) {
    return NAMA_JENIS[jenis];
}

int jumlahModel(int jenis) {
    return JUMLAH_MODEL[jenis];
}

string namaModel(int jenis, int nomor) {
    if (nomor < 1 || nomor > JUMLAH_MODEL[jenis]) return "";
    return MODEL[jenis][nomor - 1];
}

bool modelValid(int jenis, string model) {
    for (int i = 1; i <= jumlahModel(jenis); i++) {
        if (namaModel(jenis, i) == model) return true;
    }
    return false;
}

// Mencari jenis kendaraan terkecil yang masih muat
bool jenisUntukKursi(int jumlahKursi, int& hasil) {
    for (int i = 0; i < JUMLAH_JENIS; i++) {
        if (jumlahKursi <= KAPASITAS[i]) {
            hasil = i;
            return true;
        }
    }
    return false;
}

int kursiAnggota(DataOrang d) {
    return d.umur >= 2 ? 1 : 0;
}

int hitungKursi(Pemesanan* p) {
    return 1 + kursiDaftar(p->daftarPenumpang);  // pemesan selalu dihitung
}

int hitungTercatat(Pemesanan* p) {
    int total = 1;  // pemesan
    for (Penumpang* q = p->daftarPenumpang; q != nullptr; q = q->next) {
        total++;
    }
    return total;
}

string namaStatus(int status) {
    if (status == BELUM_LUNAS) return "Belum lunas";
    if (status == LUNAS)       return "Lunas";
    return "Batal";
}

void cetakDetailPemesanan(Pemesanan* p) {
    cout << "Kode booking  : " << p->kodeBooking << "\n";
    cout << "Pemesan       : " << p->pemesan.nama << " (" << p->pemesan.umur
         << " th, " << p->pemesan.jk << ")\n";
    cout << "Kendaraan     : " << namaJenis(p->jenis) << " - " << p->model
         << " (kapasitas " << kapasitasJenis(p->jenis) << ")\n";
    cout << "Berangkat     : " << formatTanggal(p->tanggalBerangkat) << "\n";
    cout << "Kursi terpakai: " << hitungKursi(p) << "\n";
    cout << "Total tercatat: " << hitungTercatat(p) << "\n";
    cout << "Status        : " << namaStatus(p->status) << "\n";
    cout << "Anggota rombongan:\n";
    if (p->daftarPenumpang == nullptr) {
        cout << "  (tidak ada anggota selain pemesan)\n";
    }
    int no = 1;
    for (Penumpang* q = p->daftarPenumpang; q != nullptr; q = q->next) {
        cout << "  " << no++ << ". " << q->data.nama << " (" << q->data.umur
             << " th, " << q->data.jk << ")\n";
    }
    cout << "Riwayat:\n";
    for (Catatan* c = p->riwayat; c != nullptr; c = c->next) {
        cout << "  - " << c->teks << "\n";
    }
}

Penumpang* sisipAkhirPenumpang(Penumpang* kepala, DataOrang d) {
    Penumpang* baru = new Penumpang{d, nullptr};
    if (kepala == nullptr) return baru;
    Penumpang* q = kepala;
    while (q->next != nullptr) q = q->next;
    q->next = baru;
    return kepala;
}

void hapusDaftarPenumpang(Penumpang* kepala) {
    while (kepala) {
        Penumpang* nx = kepala->next;
        delete kepala;
        kepala = nx;
    }
}

// ===== Inisialisasi dan pembersihan data =====

void inisialisasiSistem() {
    kepala = nullptr;
    idTerakhir = 0;
}

void bersihkanSistem() {
    Kategori* k = kepala;
    while (k) {
        TempatWisata* t = k->daftarTempat;
        while (t) {
            TempatWisata* tNext = t->next;
            hapusSemuaPemesanan(t->daftarPemesanan);
            delete t;
            t = tNext;
        }
        Kategori* kNext = k->next;
        delete k;
        k = kNext;
    }
    kepala = nullptr;
}

// ===== Pencarian internal =====

Kategori* cariKategori(string nama) {
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        if (k->nama == nama) return k;
    }
    return nullptr;
}

TempatWisata* cariTempat(string namaKategori, string namaTempat) {
    Kategori* k = cariKategori(namaKategori);
    if (!k) return nullptr;
    for (TempatWisata* t = k->daftarTempat; t != nullptr; t = t->next) {
        if (t->nama == namaTempat) return t;
    }
    return nullptr;
}

// ===== Pengelolaan tempat wisata =====

bool tambahKategori(string nama) {
    if (nama.empty() || cariKategori(nama) != nullptr) return false;

    Kategori* baru = new Kategori{nama, nullptr, nullptr};
    if (kepala == nullptr) {
        kepala = baru;
    } else {
        Kategori* k = kepala;
        while (k->next) k = k->next;
        k->next = baru;
    }
    return true;
}

bool tambahTempat(string namaKategori, string namaTempat) {
    Kategori* k = cariKategori(namaKategori);
    if (!k || namaTempat.empty() || cariTempat(namaKategori, namaTempat) != nullptr) {
        return false;
    }

    TempatWisata* baru = new TempatWisata{namaTempat, nullptr, nullptr};
    if (k->daftarTempat == nullptr) {
        k->daftarTempat = baru;
    } else {
        TempatWisata* t = k->daftarTempat;
        while (t->next) t = t->next;
        t->next = baru;
    }
    return true;
}

bool hapusTempat(string namaKategori, string namaTempat, string& pesan) {
    Kategori* k = cariKategori(namaKategori);
    if (!k) {
        pesan = "Kategori tidak ditemukan.";
        return false;
    }

    TempatWisata* prev = nullptr;
    TempatWisata* t = k->daftarTempat;
    while (t && t->nama != namaTempat) {
        prev = t;
        t = t->next;
    }
    if (!t) {
        pesan = "Tempat wisata tidak ditemukan.";
        return false;
    }

    for (Pemesanan* p = t->daftarPemesanan; p != nullptr; p = p->next) {
        if (p->status != BATAL) {
            pesan = "Tempat tidak bisa dihapus karena masih ada pemesanan aktif.";
            return false;
        }
    }

    // Tersisa hanya pemesanan batal, ikut dibersihkan
    hapusSemuaPemesanan(t->daftarPemesanan);
    if (prev) prev->next = t->next;
    else k->daftarTempat = t->next;
    delete t;

    pesan = "Tempat wisata berhasil dihapus.";
    return true;
}

void tampilkanKatalog() {
    if (kepala == nullptr) {
        cout << "Katalog masih kosong.\n";
        return;
    }
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        cout << "[" << k->nama << "]\n";
        if (k->daftarTempat == nullptr) {
            cout << "   (belum ada tempat wisata)\n";
        }
        for (TempatWisata* t = k->daftarTempat; t != nullptr; t = t->next) {
            cout << "   - " << t->nama << "\n";
        }
    }
}

int cetakNomorKategori() {
    int no = 0;
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        no++;
        cout << no << ". " << k->nama << "\n";
    }
    return no;
}

string kategoriKe(int nomor) {
    int no = 0;
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        if (++no == nomor) return k->nama;
    }
    return "";
}

int cetakNomorTempat(string namaKategori) {
    Kategori* k = cariKategori(namaKategori);
    if (!k) return 0;
    int no = 0;
    for (TempatWisata* t = k->daftarTempat; t != nullptr; t = t->next) {
        no++;
        cout << no << ". " << t->nama << "\n";
    }
    return no;
}

string tempatKe(string namaKategori, int nomor) {
    Kategori* k = cariKategori(namaKategori);
    if (!k) return "";
    int no = 0;
    for (TempatWisata* t = k->daftarTempat; t != nullptr; t = t->next) {
        if (++no == nomor) return t->nama;
    }
    return "";
}

// ===== Pencarian =====

Pemesanan* cariPemesanan(string kode) {
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        for (TempatWisata* t = k->daftarTempat; t != nullptr; t = t->next) {
            for (Pemesanan* p = t->daftarPemesanan; p != nullptr; p = p->next) {
                if (p->kodeBooking == kode) return p;
            }
        }
    }
    return nullptr;
}

void tampilkanWisataTerurut() {
    Item* daftar = nullptr;
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        for (TempatWisata* t = k->daftarTempat; t != nullptr; t = t->next) {
            Item* baru = new Item{t->nama + " (" + k->nama + ")", t->nama, 0, nullptr};
            sisipUrut(daftar, baru, sebelumNama);
        }
    }
    if (daftar == nullptr) {
        cout << "Belum ada tempat wisata.\n";
        return;
    }
    int no = 1;
    for (Item* it = daftar; it != nullptr; it = it->next) {
        cout << no++ << ". " << it->label << "\n";
    }
    hapusDaftarItem(daftar);
}

// ===== Pemesanan =====

bool buatPemesanan(string namaKategori, string namaTempat,
                                 DataOrang pemesan, Penumpang* anggota,
                                 int jenis, string model,
                                 Tanggal tanggal,
                                 string& kodeHasil, string& pesan) {
    TempatWisata* t = cariTempat(namaKategori, namaTempat);
    string err;
    if (!t) {
        err = "Tempat wisata tidak ditemukan.";
    } else if (pemesan.nama.empty()) {
        err = "Nama pemesan tidak boleh kosong.";
    } else if (selisihHari(hariIni(), tanggal) < 0) {
        err = "Tanggal keberangkatan sudah lewat.";
    } else if (1 + kursiDaftar(anggota) > kapasitasJenis(jenis)) {
        err = "Jumlah kursi melebihi kapasitas kendaraan.";
    } else if (!modelValid(jenis, model)) {
        err = "Model tidak tersedia untuk jenis kendaraan ini.";
    }

    if (!err.empty()) {
        hapusDaftarPenumpang(anggota);  // daftar anggota tidak jadi dipakai
        pesan = err;
        return false;
    }

    Pemesanan* baru = new Pemesanan();
    baru->idInternal = ++idTerakhir;
    baru->kodeBooking = buatKode(baru->idInternal);
    baru->pemesan = pemesan;
    baru->jenis = jenis;
    baru->model = model;
    baru->tanggalBerangkat = tanggal;
    baru->status = BELUM_LUNAS;
    baru->riwayat = nullptr;
    baru->daftarPenumpang = anggota;  // daftar anggota dipindahkan ke pemesanan
    baru->next = nullptr;

    tambahCatatan(baru, "Booking dibuat: " + namaJenis(jenis) + " (" + model +
                        "), total " + to_string(hitungTercatat(baru)) + " orang.");

    // Sisipkan di akhir cabang pemesanan milik tempat ini
    if (t->daftarPemesanan == nullptr) {
        t->daftarPemesanan = baru;
    } else {
        Pemesanan* q = t->daftarPemesanan;
        while (q->next) q = q->next;
        q->next = baru;
    }

    kodeHasil = baru->kodeBooking;
    pesan = "Booking berhasil.";
    return true;
}

bool tambahPenumpang(string kode, DataOrang anggota,
                                   int jenisBaru, string modelBaru,
                                   string& pesan) {
    Pemesanan* p = cariPemesanan(kode);
    if (!p) {
        pesan = "Kode booking tidak ditemukan.";
        return false;
    }
    if (p->status == BATAL) {
        pesan = "Pemesanan sudah dibatalkan.";
        return false;
    }

    int kursiBaru = hitungKursi(p) + kursiAnggota(anggota);
    if (kursiBaru > kapasitasJenis(jenisBaru)) {
        pesan = "Jumlah kursi melebihi kapasitas kendaraan yang dipilih.";
        return false;
    }

    string catatan = "Tambah penumpang: " + anggota.nama + " (" + to_string(anggota.umur) + " th)";
    bool gantiKendaraan = (jenisBaru != p->jenis) || (modelBaru != p->model);

    if (gantiKendaraan) {
        catatan += ". Kendaraan diganti dari " + namaJenis(p->jenis) + " (" + p->model +
                   ") ke " + namaJenis(jenisBaru) + " (" + modelBaru + ")";
        if (p->status == LUNAS) {
            p->status = BELUM_LUNAS;
            catatan += ", status menjadi belum lunas";
        }
        catatan += ".";
        p->jenis = jenisBaru;
        p->model = modelBaru;
    }

    p->daftarPenumpang = sisipAkhirPenumpang(p->daftarPenumpang, anggota);
    tambahCatatan(p, catatan);

    pesan = "Penumpang berhasil ditambahkan.";
    return true;
}

bool konfirmasiBayar(string kode, string& pesan) {
    Pemesanan* p = cariPemesanan(kode);
    if (!p) {
        pesan = "Kode booking tidak ditemukan.";
        return false;
    }
    if (p->status == BATAL) {
        pesan = "Pemesanan sudah dibatalkan, pembayaran tidak bisa dikonfirmasi.";
        return false;
    }
    if (p->status == LUNAS) {
        pesan = "Pemesanan ini sudah lunas.";
        return false;
    }

    p->status = LUNAS;
    tambahCatatan(p, "Pembayaran dikonfirmasi (lunas).");
    pesan = "Pembayaran berhasil dikonfirmasi.";
    return true;
}

bool batalkanPemesanan(string kode, string& pesan) {
    Pemesanan* p = cariPemesanan(kode);
    if (!p) {
        pesan = "Kode booking tidak ditemukan.";
        return false;
    }
    if (p->status == BATAL) {
        pesan = "Pemesanan sudah dibatalkan sebelumnya.";
        return false;
    }

    int sisa = selisihHari(hariIni(), p->tanggalBerangkat);
    if (sisa < 0) {
        pesan = "Tanggal keberangkatan sudah lewat, pembatalan tidak bisa dilakukan.";
        return false;
    }
    if (sisa < 3) {
        pesan = "Pembatalan hanya bisa dilakukan minimal H-3 sebelum keberangkatan. "
                "Sisa waktu: " + to_string(sisa) + " hari.";
        return false;
    }

    p->status = BATAL;
    tambahCatatan(p, "Pemesanan dibatalkan.");
    pesan = "Pemesanan berhasil dibatalkan.";
    return true;
}

bool tampilkanPemesananPerWisata(string namaKategori, string namaTempat) {
    TempatWisata* t = cariTempat(namaKategori, namaTempat);
    if (!t) return false;

    if (t->daftarPemesanan == nullptr) {
        cout << "Belum ada pemesanan untuk tempat ini.\n";
        return true;
    }

    int no = 1;
    for (Pemesanan* p = t->daftarPemesanan; p != nullptr; p = p->next) {
        cout << no++ << ". " << p->kodeBooking
             << " | " << p->pemesan.nama
             << " | " << namaJenis(p->jenis) << " - " << p->model
             << " | " << hitungTercatat(p) << " orang"
             << " | " << namaStatus(p->status) << "\n";
    }
    return true;
}

// ===== Statistik =====

void tampilkanStatistik() {
    Item* daftar = nullptr;
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        for (TempatWisata* t = k->daftarTempat; t != nullptr; t = t->next) {
            Item* baru = new Item{t->nama + " (" + k->nama + ")", "", hitungAktif(t), nullptr};
            sisipUrut(daftar, baru, sebelumNilai);
        }
    }

    if (daftar == nullptr) {
        cout << "Belum ada tempat wisata.\n";
        return;
    }

    cout << "Peringkat tempat wisata (pemesanan aktif):\n";
    int no = 1;
    for (Item* it = daftar; it != nullptr; it = it->next) {
        cout << no++ << ". " << it->label << " - " << it->nilai << " pemesanan\n";
    }
    if (daftar->nilai > 0) {
        cout << "\nPaling ramai: " << daftar->label << "\n";
    }
    hapusDaftarItem(daftar);
}

