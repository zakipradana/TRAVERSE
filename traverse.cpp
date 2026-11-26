#include "traverse.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>

using namespace std;

// State sistem (hanya terlihat di file ini)
static Kategori* kepala = nullptr;              // MLL: list utama kategori
static NodeBST* akarBooking = nullptr;          // [BST] indeks kode booking
static NodeBST* akarTempat = nullptr;           // [BST] indeks "nama (kategori)"
static NodeAntrean* antreanDepan = nullptr;     // [QUEUE] sisi depan antrean
static NodeAntrean* antreanBelakang = nullptr;  // [QUEUE] sisi belakang antrean
static Aksi* tumpukan = nullptr;                // [STACK] puncak stack undo
static int idTerakhir = 0;

namespace {

// Konstanta dan data kendaraan (kode awal)

// [H-3] Pembayaran, pembatalan, perubahan, dan undo hanya boleh sampai H-3
const int BATAS_H_MIN = 3;

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

// Item sementara untuk statistik (linked list sementara)
struct Item {
    string label;
    int nilai;
    Item* next;
};

//Kode dan tanggal

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

// [H-3] Sisa hari dari hari ini sampai keberangkatan
int sisaHari(Pemesanan* p) {
    return selisihHari(hariIni(), p->tanggalBerangkat);
}

// [H-3] Sisa hari minimal 3: masih boleh bayar, batal, ubah, atau undo
bool masihDalamBatas(Pemesanan* p) {
    return sisaHari(p) >= BATAS_H_MIN;
}

// [BST] Kunci indeks tempat: "nama (kategori)" agar unik antar kategori
string kunciTempat(string kat, string nama) {
    return nama + " (" + kat + ")";
}

// ----- Catatan, daftar, dan penumpang (kode awal) -----

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
int kursiDaftar(Penumpang* awal) {
    int total = 0;
    for (Penumpang* q = awal; q != nullptr; q = q->next) {
        total += kursiAnggota(q->data);
    }
    return total;
}

// Jumlah pemesanan aktif (tidak batal) pada satu tempat
int hitungAktif(TempatWisata* t) {
    int aktif = 0;
    for (Pemesanan* p = t->daftarPemesanan; p != nullptr; p = p->next) {
        if (p->status != BATAL) aktif++;
    }
    return aktif;
}

// [STACK] Menghapus anggota paling belakang (dipakai saat undo tambah penumpang)
void hapusPenumpangTerakhir(Pemesanan* p) {
    if (p->daftarPenumpang == nullptr) return;
    if (p->daftarPenumpang->next == nullptr) {
        delete p->daftarPenumpang;
        p->daftarPenumpang = nullptr;
        return;
    }
    Penumpang* q = p->daftarPenumpang;
    while (q->next->next != nullptr) q = q->next;
    delete q->next;
    q->next = nullptr;
}

// ----- [BST] AVL -----

int tinggiNode(const NodeBST* n) {
    return n ? n->tinggi : 0;
}

void perbaruiTinggi(NodeBST* n) {
    n->tinggi = 1 + max(tinggiNode(n->kiri), tinggiNode(n->kanan));
}

// [BST] Rotasi kanan (kasus kiri-kiri)
NodeBST* rotasiKanan(NodeBST* y) {
    NodeBST* x = y->kiri;
    NodeBST* t2 = x->kanan;
    x->kanan = y;
    y->kiri = t2;
    perbaruiTinggi(y);
    perbaruiTinggi(x);
    return x;
}

// [BST] Rotasi kiri (kasus kanan-kanan)
NodeBST* rotasiKiri(NodeBST* x) {
    NodeBST* y = x->kanan;
    NodeBST* t2 = y->kiri;
    y->kiri = x;
    x->kanan = t2;
    perbaruiTinggi(x);
    perbaruiTinggi(y);
    return y;
}

// [BST] Menyeimbangkan satu node setelah sisip atau hapus
NodeBST* seimbangkan(NodeBST* n) {
    perbaruiTinggi(n);
    int bal = tinggiNode(n->kiri) - tinggiNode(n->kanan);
    if (bal > 1) {
        if (tinggiNode(n->kiri->kiri) < tinggiNode(n->kiri->kanan))
            n->kiri = rotasiKiri(n->kiri);
        return rotasiKanan(n);
    }
    if (bal < -1) {
        if (tinggiNode(n->kanan->kanan) < tinggiNode(n->kanan->kiri))
            n->kanan = rotasiKanan(n->kanan);
        return rotasiKiri(n);
    }
    return n;
}

// [BST] Menyisipkan node baru. Kunci harus unik (dicek oleh pemanggil).
NodeBST* sisipBST(NodeBST* n, const string& kunci, Pemesanan* p, TempatWisata* t) {
    if (!n) {
        NodeBST* baru = new NodeBST();
        baru->kunci = kunci;
        baru->pemesanan = p;
        baru->tempat = t;
        baru->kiri = nullptr;
        baru->kanan = nullptr;
        baru->tinggi = 1;
        return baru;
    }
    if (kunci < n->kunci) n->kiri = sisipBST(n->kiri, kunci, p, t);
    else if (kunci > n->kunci) n->kanan = sisipBST(n->kanan, kunci, p, t);
    else return n;
    return seimbangkan(n);
}

// [BST] Pencarian biner: O(log n) karena pohon seimbang
NodeBST* cariBST(NodeBST* n, const string& kunci) {
    while (n) {
        if (kunci == n->kunci) return n;
        n = (kunci < n->kunci) ? n->kiri : n->kanan;
    }
    return nullptr;
}

// [BST] Node terkecil di subpohon, dipakai saat hapus
NodeBST* nodeTerkecil(NodeBST* n) {
    while (n->kiri) n = n->kiri;
    return n;
}

// [BST] Hapus node dengan kunci tertentu, lalu seimbangkan ulang
NodeBST* hapusBST(NodeBST* n, const string& kunci) {
    if (!n) return nullptr;
    if (kunci < n->kunci) {
        n->kiri = hapusBST(n->kiri, kunci);
    } else if (kunci > n->kunci) {
        n->kanan = hapusBST(n->kanan, kunci);
    } else {
        if (!n->kiri || !n->kanan) {
            NodeBST* sisa = n->kiri ? n->kiri : n->kanan;
            delete n;
            return sisa;
        }
        NodeBST* pengganti = nodeTerkecil(n->kanan);
        string k = pengganti->kunci;  // salin dulu, node pengganti akan dihapus
        n->kunci = k;
        n->pemesanan = pengganti->pemesanan;
        n->tempat = pengganti->tempat;
        n->kanan = hapusBST(n->kanan, k);
    }
    return seimbangkan(n);
}

// [BST] Hanya membebaskan node indeks. Data pemesanan dan tempat tidak ikut dihapus.
void hapusSemuaBST(NodeBST* n) {
    if (!n) return;
    hapusSemuaBST(n->kiri);
    hapusSemuaBST(n->kanan);
    delete n;
}

// [BST] Inorder menghasilkan urutan kunci yang sudah terurut
void cetakInorder(NodeBST* n, int& no) {
    if (!n) return;
    cetakInorder(n->kiri, no);
    cout << no++ << ". " << n->kunci << "\n";
    cetakInorder(n->kanan, no);
}

// [QUEUE] Antrean konfirmasi pembayaran
// Isi antrean = pemesanan yang statusnya BELUM_LUNAS, urut sesuai waktu masuk

// [QUEUE] Masuk ke belakang antrean
void enqueue(Pemesanan* p) {
    NodeAntrean* baru = new NodeAntrean{p, nullptr};
    if (antreanBelakang) antreanBelakang->next = baru;
    else antreanDepan = baru;
    antreanBelakang = baru;
}

// [QUEUE] Cek apakah pemesanan sudah ada di antrean
bool adaDiAntrean(Pemesanan* p) {
    for (NodeAntrean* n = antreanDepan; n; n = n->next) {
        if (n->data == p) return true;
    }
    return false;
}

// [QUEUE] Mengeluarkan pemesanan dari posisi mana pun, urutan yang lain tetap
void hapusDariAntrean(Pemesanan* p) {
    NodeAntrean* prev = nullptr;
    NodeAntrean* n = antreanDepan;
    while (n && n->data != p) {
        prev = n;
        n = n->next;
    }
    if (!n) return;
    if (prev) prev->next = n->next;
    else antreanDepan = n->next;
    if (antreanBelakang == n) antreanBelakang = prev;
    delete n;
}

// [QUEUE] Aturan: pemesanan ada di antrean jika dan hanya jika statusnya BELUM_LUNAS
void sinkronAntrean(Pemesanan* p) {
    bool ada = adaDiAntrean(p);
    if (p->status == BELUM_LUNAS && !ada) enqueue(p);
    else if (p->status != BELUM_LUNAS && ada) hapusDariAntrean(p);
}

// ----- [STACK] Aksi untuk undo -----

// [STACK] Menyimpan keadaan sebelum perubahan, lalu mendorongnya ke puncak stack
void catatAksi(int tipe, string deskripsi, Pemesanan* p, int statusLama, int jenisLama, string modelLama) {
    Aksi* a = new Aksi{tipe, deskripsi, p, statusLama, jenisLama, modelLama, nullptr};
    a->next = tumpukan;
    tumpukan = a;
}

// [STACK] Membuang aksi milik satu pemesanan, dipakai sebelum pemesanan dibebaskan
void bersihkanAksiUntuk(Pemesanan* p) {
    Aksi** pp = &tumpukan;
    while (*pp) {
        if ((*pp)->p == p) {
            Aksi* hapus = *pp;
            *pp = hapus->next;
            delete hapus;
        } else {
            pp = &(*pp)->next;
        }
    }
}

// [STACK] Mengosongkan seluruh stack
void hapusSemuaAksi() {
    while (tumpukan) {
        Aksi* nx = tumpukan->next;
        delete tumpukan;
        tumpukan = nx;
    }
}

// [STACK][QUEUE] Konfirmasi pembayaran, dipakai manual dan dari antrean.
void konfirmasiInternal(Pemesanan* p, string deskripsi) {
    catatAksi(AKSI_KONFIRMASI, deskripsi, p, p->status, p->jenis, p->model);
    p->status = LUNAS;
    tambahCatatan(p, "Pembayaran dikonfirmasi (lunas).");
    sinkronAntrean(p);
}

// Urutan item statistik (linked list sementara)

// Jumlah pemesanan terbanyak dulu, lalu nama
bool lebihDulu(Item* a, Item* b) {
    if (a->nilai != b->nilai) return a->nilai > b->nilai;
    return a->label < b->label;
}

// Sisip terurut untuk statistik
void sisipUrut(Item*& daftar, Item* baru) {
    if (!daftar || lebihDulu(baru, daftar)) {
        baru->next = daftar;
        daftar = baru;
        return;
    }
    Item* p = daftar;
    while (p->next && !lebihDulu(baru, p->next)) p = p->next;
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

//Fungsi bantu kendaraan dan tampilan (publik, sesuai traverse.h)

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
    if (status == LUNAS) return "Lunas";
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

Penumpang* sisipAkhirPenumpang(Penumpang* awal, DataOrang d) {
    Penumpang* baru = new Penumpang{d, nullptr};
    if (awal == nullptr) return baru;
    Penumpang* q = awal;
    while (q->next != nullptr) q = q->next;
    q->next = baru;
    return awal;
}

void hapusDaftarPenumpang(Penumpang* awal) {
    while (awal) {
        Penumpang* nx = awal->next;
        delete awal;
        awal = nx;
    }
}

// ===== Inisialisasi dan pembersihan =====

// [UBAH] Menginisialisasi juga BST, queue, dan stack
void inisialisasiSistem() {
    kepala = nullptr;
    akarBooking = nullptr;      // [BST]
    akarTempat = nullptr;       // [BST]
    antreanDepan = nullptr;     // [QUEUE]
    antreanBelakang = nullptr;  // [QUEUE]
    tumpukan = nullptr;         // [STACK]
    idTerakhir = 0;
}

// [UBAH] Membersihkan BST, queue, dan stack sebelum MLL dibebaskan
void bersihkanSistem() {
    // [BST] Indeks hanya menyimpan penunjuk, jadi dibersihkan dulu
    hapusSemuaBST(akarBooking);
    akarBooking = nullptr;
    hapusSemuaBST(akarTempat);
    akarTempat = nullptr;

    // [QUEUE] Antrean hanya menyimpan penunjuk
    while (antreanDepan) {
        NodeAntrean* n = antreanDepan->next;
        delete antreanDepan;
        antreanDepan = n;
    }
    antreanBelakang = nullptr;

    // [STACK] Stack hanya menyimpan penunjuk
    hapusSemuaAksi();

    // MLL: data sebenarnya dibebaskan di sini
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

// Pencarian internal 

// MLL: pencarian kategori (kode awal)
Kategori* cariKategori(string nama) {
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        if (k->nama == nama) return k;
    }
    return nullptr;
}

// [UBAH][BST] Pencarian tempat lewat indeks AVL, bukan penelusuran MLL
TempatWisata* cariTempat(string namaKategori, string namaTempat) {
    NodeBST* n = cariBST(akarTempat, kunciTempat(namaKategori, namaTempat));
    return n ? n->tempat : nullptr;
}

//Pengelolaan tempat wisata

// MLL: menambah kategori di akhir list (kode awal)
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

// [UBAH][BST] Menambah tempat ke MLL dan juga ke indeks AVL
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
    // [BST] Daftarkan juga ke indeks nama tempat
    akarTempat = sisipBST(akarTempat, kunciTempat(namaKategori, namaTempat), nullptr, baru);
    return true;
}

// [UBAH][BST][STACK][QUEUE] Hapus tempat hanya jika semua pemesanannya batal
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

    // Semua pemesanan di sini sudah batal. Lepaskan dari BST dan stack dulu.
    for (Pemesanan* p = t->daftarPemesanan; p != nullptr; p = p->next) {
        akarBooking = hapusBST(akarBooking, p->kodeBooking);  // [BST]
        bersihkanAksiUntuk(p);                                // [STACK]
        // [QUEUE] Pemesanan batal sudah keluar dari antrean lewat sinkronAntrean
    }
    hapusSemuaPemesanan(t->daftarPemesanan);

    akarTempat = hapusBST(akarTempat, kunciTempat(namaKategori, namaTempat));  // [BST]
    if (prev) prev->next = t->next;
    else k->daftarTempat = t->next;
    delete t;

    pesan = "Tempat wisata berhasil dihapus.";
    return true;
}

// MLL: menampilkan katalog (kode awal)
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

// MLL: daftar bernomor untuk menu (kode awal)
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

// Pencarian (BST)

// [UBAH][BST] Cari Tiket: pencarian O(log n) lewat indeks kode booking
Pemesanan* cariPemesanan(string kode) {
    NodeBST* n = cariBST(akarBooking, kode);
    return n ? n->pemesanan : nullptr;
}

// [UBAH][BST] Daftar Wisata Terurut: inorder pada indeks tempat
void tampilkanWisataTerurut() {
    if (akarTempat == nullptr) {
        cout << "Belum ada tempat wisata.\n";
        return;
    }
    int no = 1;
    cetakInorder(akarTempat, no);
}

//Pemesanan 

// [UBAH][BST][QUEUE][H-3] Booking: MLL + indeks BST + masuk antrean konfirmasi
bool buatPemesanan(string namaKategori, string namaTempat, DataOrang pemesan, Penumpang* anggota,
                   int jenis, string model, Tanggal tanggal, string& kodeHasil, string& pesan) {
    TempatWisata* t = cariTempat(namaKategori, namaTempat);
    int sisa = selisihHari(hariIni(), tanggal);
    string err;
    if (!t) {
        err = "Tempat wisata tidak ditemukan.";
    } else if (pemesan.nama.empty()) {
        err = "Nama pemesan tidak boleh kosong.";
    } else if (sisa < 0) {
        err = "Tanggal keberangkatan sudah lewat.";
    } else if (sisa < BATAS_H_MIN) {                    // [H-3]
        err = "Booking tidak bisa dibuat: batas pembayaran H-3 untuk tanggal ini sudah lewat.";
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
    baru->daftarPenumpang = anggota;
    baru->next = nullptr;

    tambahCatatan(baru, "Booking dibuat: " + namaJenis(jenis) + " (" + model + "), total " + to_string(hitungTercatat(baru)) + " orang.");

    // MLL: sisip di akhir cabang pemesanan milik tempat ini
    if (t->daftarPemesanan == nullptr) {
        t->daftarPemesanan = baru;
    } else {
        Pemesanan* q = t->daftarPemesanan;
        while (q->next) q = q->next;
        q->next = baru;
    }

    // [BST] Indeks kode booking
    akarBooking = sisipBST(akarBooking, baru->kodeBooking, baru, nullptr);

    // [QUEUE] Masuk antrean konfirmasi karena belum lunas
    sinkronAntrean(baru);

    kodeHasil = baru->kodeBooking;
    pesan = "Booking berhasil.";
    return true;
}

// [UBAH][STACK][QUEUE][H-3] Tambah penumpang: dicatat ke stack untuk undo
bool tambahPenumpang(string kode, DataOrang anggota, int jenisBaru, string modelBaru, string& pesan) {
    Pemesanan* p = cariPemesanan(kode);  // [BST]
    if (!p) {
        pesan = "Kode booking tidak ditemukan.";
        return false;
    }
    if (p->status == BATAL) {
        pesan = "Pemesanan sudah dibatalkan.";
        return false;
    }
    if (!masihDalamBatas(p)) {           // [H-3]
        pesan = "Penambahan penumpang hanya bisa dilakukan sampai H-3 sebelum keberangkatan.";
        return false;
    }

    int kursiBaru = hitungKursi(p) + kursiAnggota(anggota);
    if (kursiBaru > kapasitasJenis(jenisBaru)) {
        pesan = "Jumlah kursi melebihi kapasitas kendaraan yang dipilih.";
        return false;
    }

    string catatan = "Tambah penumpang: " + anggota.nama + " (" + to_string(anggota.umur) + " th)";
    bool gantiKendaraan = (jenisBaru != p->jenis) || (modelBaru != p->model);

    // [STACK] Simpan keadaan sebelum perubahan untuk undo
    catatAksi(AKSI_TAMBAH, "tambah penumpang " + anggota.nama + " ke " + p->kodeBooking,
              p, p->status, p->jenis, p->model);

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

    p->daftarPenumpang = sisipAkhirPenumpang(p->daftarPenumpang, anggota);  // MLL
    tambahCatatan(p, catatan);
    sinkronAntrean(p);  // [QUEUE] jika kembali belum lunas, masuk lagi ke antrean

    pesan = "Penumpang berhasil ditambahkan.";
    return true;
}

// [UBAH][QUEUE][H-3] Konfirmasi manual pembayaran
bool konfirmasiBayar(string kode, string& pesan) {
    Pemesanan* p = cariPemesanan(kode);  // [BST]
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
    if (!masihDalamBatas(p)) {           // [H-3]
        pesan = "Batas pembayaran H-3 sudah lewat, pemesanan ini sudah dibatalkan otomatis.";
        return false;
    }

    konfirmasiInternal(p, "konfirmasi pembayaran " + p->kodeBooking);  // [STACK][QUEUE]
    pesan = "Pembayaran berhasil dikonfirmasi.";
    return true;
}

// [UBAH][QUEUE] Batalkan pemesanan: keluar dari antrean
bool batalkanPemesanan(string kode, string& pesan) {
    Pemesanan* p = cariPemesanan(kode);  // [BST]
    if (!p) {
        pesan = "Kode booking tidak ditemukan.";
        return false;
    }
    if (p->status == BATAL) {
        pesan = "Pemesanan sudah dibatalkan sebelumnya.";
        return false;
    }

    int sisa = sisaHari(p);  // [H-3]
    if (sisa < 0) {
        pesan = "Tanggal keberangkatan sudah lewat, pembatalan tidak bisa dilakukan.";
        return false;
    }
    if (sisa < BATAS_H_MIN) {
        pesan = "Pembatalan hanya bisa dilakukan minimal H-3 sebelum keberangkatan. "
                "Sisa waktu: " + to_string(sisa) + " hari.";
        return false;
    }

    catatAksi(AKSI_BATAL, "pembatalan " + p->kodeBooking, p, p->status, p->jenis, p->model);  // [STACK]
    p->status = BATAL;
    tambahCatatan(p, "Pemesanan dibatalkan.");
    sinkronAntrean(p);  // [QUEUE] keluar dari antrean
    pesan = "Pemesanan berhasil dibatalkan.";
    return true;
}

// MLL: lihat pemesanan per wisata (kode awal)
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

// Statistik

// MLL: menghitung pemesanan aktif per tempat, lalu diurutkan dengan linked list sementara
void tampilkanStatistik() {
    Item* daftar = nullptr;
    for (Kategori* k = kepala; k != nullptr; k = k->next) {
        for (TempatWisata* t = k->daftarTempat; t != nullptr; t = t->next) {
            Item* baru = new Item{kunciTempat(k->nama, t->nama), hitungAktif(t), nullptr};
            sisipUrut(daftar, baru);
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

// [H-3] Jatuh tempo otomatis

// [H-3][QUEUE][STACK] Dipanggil sebelum setiap menu.
// Pemesanan yang belum lunas dan sudah lewat H-3 dibatalkan otomatis.
// Aksi undo yang sudah lewat H-3 dibuang dari stack.
int jalankanJatuhTempo() {
    int dibatalkan = 0;

    // [QUEUE] Telusuri antrean. Simpan next dulu karena node bisa dilepas.
    NodeAntrean* n = antreanDepan;
    while (n) {
        NodeAntrean* nx = n->next;
        Pemesanan* p = n->data;
        if (!masihDalamBatas(p)) {
            p->status = BATAL;
            tambahCatatan(p, "Dibatalkan otomatis: belum lunas sampai batas H-3.");
            sinkronAntrean(p);       // [QUEUE] keluar dari antrean
            bersihkanAksiUntuk(p);   // [STACK] aksi pemesanan ini tidak boleh di-undo lagi
            cout << "[Otomatis] " << p->kodeBooking << " (" << p->pemesan.nama
                 << ") dibatalkan karena belum lunas sampai H-3.\n";
            dibatalkan++;
        }
        n = nx;
    }

    // [STACK] Buang aksi yang sudah lewat H-3
    Aksi** pp = &tumpukan;
    while (*pp) {
        if (!masihDalamBatas((*pp)->p)) {
            Aksi* hapus = *pp;
            *pp = hapus->next;
            delete hapus;
        } else {
            pp = &(*pp)->next;
        }
    }

    return dibatalkan;
}

// [QUEUE] Antrean konfirmasi 

// [QUEUE][H-3] Memproses pemesanan terdepan lewat konfirmasiInternal
bool prosesAntreanKonfirmasi(string& pesan) {
    if (antreanDepan == nullptr) {
        pesan = "Antrean konfirmasi kosong.";
        return false;
    }
    Pemesanan* p = antreanDepan->data;
    if (!masihDalamBatas(p)) {  // [H-3]
        pesan = "Batas pembayaran H-3 sudah lewat untuk pemesanan ini.";
        return false;
    }

    // sinkronAntrean di dalam konfirmasiInternal mengeluarkannya dari antrean
    konfirmasiInternal(p, "konfirmasi antrean " + p->kodeBooking);  // [STACK][QUEUE]
    pesan = "Pembayaran " + p->kodeBooking + " dikonfirmasi dari antrean.";
    return true;
}

// [QUEUE] Menampilkan antrean dari depan ke belakang
void tampilkanAntrean() {
    if (antreanDepan == nullptr) {
        cout << "Antrean konfirmasi kosong.\n";
        return;
    }
    int no = 1;
    for (NodeAntrean* n = antreanDepan; n != nullptr; n = n->next) {
        Pemesanan* p = n->data;
        cout << no++ << ". " << p->kodeBooking
             << " | " << p->pemesan.nama
             << " | " << hitungTercatat(p) << " orang"
             << " | berangkat " << formatTanggal(p->tanggalBerangkat)
             << " | sisa " << sisaHari(p) << " hari\n";
    }
}

// =[STACK] Undo 

// [STACK][H-3] 
bool undoAksiTerakhir(string& pesan) {
    if (tumpukan == nullptr) {
        pesan = "Tidak ada aksi yang bisa di-undo.";
        return false;
    }

    Aksi* a = tumpukan;     // [STACK] ambil dari puncak
    tumpukan = a->next;
    Pemesanan* p = a->p;

    if (!masihDalamBatas(p)) {  // [H-3]
        delete a;
        pesan = "Aksi terakhir sudah melewati batas H-3, tidak bisa di-undo.";
        return false;
    }

    if (a->tipe == AKSI_TAMBAH) {
        hapusPenumpangTerakhir(p);  // penumpang yang ditambahkan selalu di belakang
        p->jenis = a->jenisLama;
        p->model = a->modelLama;
    }
    p->status = a->statusLama;
    tambahCatatan(p, "Undo: " + a->deskripsi);
    sinkronAntrean(p);  // [QUEUE] antrean mengikuti status hasil undo

    pesan = "Undo berhasil: " + a->deskripsi + " (" + p->kodeBooking + ").";
    delete a;
    return true;
}