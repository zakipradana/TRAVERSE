#ifndef TRAVEL_H
#define TRAVEL_H

#include <string>
using namespace std;

// Kode jenis kendaraan dan status pemesanan.
#define JUMLAH_JENIS 4
#define MOBIL_PRIBADI 0
#define TRAVEL 1
#define MINIBUS 2
#define BIGBUS 3
#define BELUM_LUNAS 0
#define LUNAS 1
#define BATAL 2

struct Tanggal {
    int tahun;
    int bulan;
    int hari;
};

struct DataOrang {
    string nama;
    int umur;
    string jk;
};

struct Penumpang {
    DataOrang data;
    Penumpang* next;
};

struct Catatan {
    string teks;
    Catatan* next;
};

struct Pemesanan {
    int idInternal;
    string kodeBooking;
    DataOrang pemesan;
    int jenis;
    string model;
    Tanggal tanggalBerangkat;
    int status;
    Catatan* riwayat;
    Penumpang* daftarPenumpang;
    Pemesanan* next;
};

struct TempatWisata {
    string nama;
    Pemesanan* daftarPemesanan;
    TempatWisata* next;
};

struct Kategori {
    string nama;
    TempatWisata* daftarTempat;
    Kategori* next;
};

//fungsi bantu

int kapasitasJenis(int jenis);
string namaJenis(int jenis);
int jumlahModel(int jenis);
string namaModel(int jenis, int nomor);
bool modelValid(int jenis, string model);
bool jenisUntukKursi(int jumlahKursi, int& hasil);
int kursiAnggota(DataOrang d);
int hitungKursi(Pemesanan* p);
int hitungTercatat(Pemesanan* p);
string namaStatus(int status);
void cetakDetailPemesanan(Pemesanan* p);
Penumpang* sisipAkhirPenumpang(Penumpang* kepala, DataOrang d);
void hapusDaftarPenumpang(Penumpang* kepala);

// Pengelolaan sistem travel menggunakan fungsi prosedural.
void inisialisasiSistem();
void bersihkanSistem();
Kategori* cariKategori(string nama);
TempatWisata* cariTempat(string namaKategori, string namaTempat);
bool tambahKategori(string nama);
bool tambahTempat(string namaKategori, string namaTempat);
bool hapusTempat(string namaKategori, string namaTempat, string& pesan);
void tampilkanKatalog();
int cetakNomorKategori();
string kategoriKe(int nomor);
int cetakNomorTempat(string namaKategori);
string tempatKe(string namaKategori, int nomor);
Pemesanan* cariPemesanan(string kode);
void tampilkanWisataTerurut();
bool buatPemesanan(string namaKategori, string namaTempat, DataOrang pemesan, Penumpang* anggota, int jenis, string model, Tanggal tanggal, string& kodeHasil, string& pesan);
bool tambahPenumpang(string kode, DataOrang anggota, int jenisBaru, string modelBaru, string& pesan);
bool konfirmasiBayar(string kode, string& pesan);
bool batalkanPemesanan(string kode, string& pesan);
bool tampilkanPemesananPerWisata(string namaKategori, string namaTempat);
void tampilkanStatistik();

#endif
